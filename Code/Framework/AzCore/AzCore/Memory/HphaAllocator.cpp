/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/HphaAllocator.h>
#include <AzCore/PlatformIncl.h>

#include <AzCore/Math/Random.h>
#include <AzCore/std/containers/intrusive_list.h>
#include <AzCore/std/containers/intrusive_set.h>
#include <AzCore/Memory/Memory.h>
#define _HPHA_ASSERT1(_exp) AZ_Assert(_exp, "HPHA Assert, expression: \"" #_exp "\"")
#define _HPHA_ASSERT2(_exp, _reason) AZ_Assert(_exp, "HPHA Assert, expression: \"" #_exp "\", possible reason: " #_reason)
#define _GET_MACRO12(_1, _2, NAME, ...) NAME
#define _EXPAND(x) x
#define HPPA_ASSERT(...) _EXPAND(_GET_MACRO12(__VA_ARGS__, _HPHA_ASSERT2, _HPHA_ASSERT1)(__VA_ARGS__))

#define _HPPA_ASSERT_PRINT_STACK2(_cond, _it)                                                                                              \
    {                                                                                                                                      \
        if (!_cond)                                                                                                                        \
            _it->print_stack();                                                                                                            \
        _HPHA_ASSERT1(_cond);                                                                                                              \
    }
#define _HPPA_ASSERT_PRINT_STACK3(_cond, _it, _reason)                                                                                     \
    {                                                                                                                                      \
        if (!_cond)                                                                                                                        \
            _it->print_stack();                                                                                                            \
        _HPHA_ASSERT2(_cond, _reason);                                                                                                     \
    }
#define _GET_MACRO23(_1, _2, _3, NAME, ...) NAME
#define HPPA_ASSERT_PRINT_STACK(...) _EXPAND(_GET_MACRO23(__VA_ARGS__, _HPPA_ASSERT_PRINT_STACK3, _HPPA_ASSERT_PRINT_STACK2)(__VA_ARGS__))

namespace AZ
{
    /// default windows virtual page size \todo Read this from the OS when we create the allocator)
#define OS_VIRTUAL_PAGE_SIZE AZ_PAGE_SIZE
    //////////////////////////////////////////////////////////////////////////

#define MULTITHREADED

#ifdef MULTITHREADED
#define SPIN_COUNT 4000
#endif

// Enabled mutex per bucket
#define USE_MUTEX_PER_BUCKET

    namespace HphaInternal
    {
        //! Rounds up a value to next power of 2.
        //! For example to round 8388609((2^23) + 1) up to 16777216(2^24) the following occurs
        //! Subtract one from the value in case it is already
        //! equal to a power of 2
        //! 8388609 - 1 = 8388608
        //! Propagate the highest one bit in the value to all the lower bits
        //! 8388608 = 0b100'0000'0000'0000'0000'0000 in binary
        //! 
        //!  0b100'0000'0000'0000'0000'0000 
        //! |0b010'0000'0000'0000'0000'0000 (>> 1)
        //! -------------------------------
        //!  0b110'0000'0000'0000'0000'0000 (Now there are 2 consecutive 1-bits)
        //! |0b001'1000'0000'0000'0000'0000 (>> 2)
        //! -------------------------------
        //!  0b111'1000'0000'0000'0000'0000 (Now there are 4 consecutive 1-bits)
        //! |0b000'0111'1000'0000'0000'0000 (>> 4)
        //! -------------------------------
        //!  0b111'1111'1000'0000'0000'0000 (Now there are 8 consecutive 1-bits)
        //! |0b000'0000'0111'1111'1000'0000 (>> 8)
        //! -------------------------------
        //!  0b111'1111'1111'1111'1000'0000 (Now there are 16 consecutive 1-bits)
        //! |0b000'0000'0000'0000'0111'1111 (>> 16)
        //! -------------------------------
        //!  0b111'1111'1111'1111'1111'1111 (Now there are 23 consecutive 1-bits)
        //! |0b000'0000'0000'0000'0000'0000 (>> 32)
        //! -------------------------------
        //!  0b111'1111'1111'1111'1111'1111
        //! Finally since all the one bits are set in the value, adding one pushes it
        //! to next power of 2
        //! 0b1000'0000'0000'0000'0000'0000 = 16777216
        static constexpr size_t AlignUpToPowerOfTwo(size_t value)
        {
            // If the value is <=2 it is already aligned
            if (value <= 2)
            {
                return value;
            }

            // Subtract one to make any values already
            // aligned to a power of 2 less than that power of 2
            // so that algorithm doesn't push those values upwards
            --value;
            value |= value >> 0b1;
            value |= value >> 0b10;
            value |= value >> 0b100;
            value |= value >> 0b1000;
            value |= value >> 0b1'0000;
            value |= value >> 0b10'0000;
            ++value;
            return value;
        }

        static_assert(AlignUpToPowerOfTwo(0) == 0);
        static_assert(AlignUpToPowerOfTwo(1) == 1);
        static_assert(AlignUpToPowerOfTwo(2) == 2);
        static_assert(AlignUpToPowerOfTwo(3) == 4);
        static_assert(AlignUpToPowerOfTwo(4) == 4);
        static_assert(AlignUpToPowerOfTwo(5) == 8);
        static_assert(AlignUpToPowerOfTwo(8) == 8);
        static_assert(AlignUpToPowerOfTwo(10) == 16);
        static_assert(AlignUpToPowerOfTwo(16) == 16);
        static_assert(AlignUpToPowerOfTwo(24) == 32);
        static_assert(AlignUpToPowerOfTwo(32) == 32);
        static_assert(AlignUpToPowerOfTwo(45) == 64);
        static_assert(AlignUpToPowerOfTwo(64) == 64);
        static_assert(AlignUpToPowerOfTwo(112) == 128);
        static_assert(AlignUpToPowerOfTwo(128) == 128);
        static_assert(AlignUpToPowerOfTwo(136) == 256);
        static_assert(AlignUpToPowerOfTwo(256) == 256);
    }

    //////////////////////////////////////////////////////////////////////////
    class HphaAllocatorPimpl
        : public IAllocatorWithTracking
    {
    public:
        AZ_RTTI(HphaAllocatorPimpl, "{F20122D0-93A7-4FA2-9DF9-9F0202852787}", IAllocatorWithTracking)

        // minimum allocation size, must be a power of two
        // and it needs to be able to fit a pointer
        static const size_t MIN_ALLOCATION_LOG2 = 3UL;
        static const size_t MIN_ALLOCATION = 1UL << MIN_ALLOCATION_LOG2;

        // the maximum small allocation, anything larger goes to the tree HphaAllocatorPimpl
        // must be a power of two
        static const size_t MAX_SMALL_ALLOCATION_LOG2 = 9UL;
        static const size_t MAX_SMALL_ALLOCATION = 1UL << MAX_SMALL_ALLOCATION_LOG2;

        // default alignment, must be a power of two
        static const size_t DEFAULT_ALIGNMENT = sizeof(double);

        static const size_t NUM_BUCKETS = (MAX_SMALL_ALLOCATION / MIN_ALLOCATION);

        static inline bool is_small_allocation(size_t s)
        {
            return s <= MAX_SMALL_ALLOCATION;
        }
        static inline size_t clamp_small_allocation(size_t s)
        {
            return (s < MIN_ALLOCATION) ? MIN_ALLOCATION : s;
        }

        // bucket spacing functions control how the size-space is divided between buckets
        // currently we use linear spacing, could be changed to logarithmic etc
        static inline unsigned bucket_spacing_function(size_t size)
        {
            return (unsigned)((size + (MIN_ALLOCATION - 1)) >> MIN_ALLOCATION_LOG2) - 1;
        }
        static inline unsigned bucket_spacing_function_aligned(size_t size)
        {
            return (unsigned)(size >> MIN_ALLOCATION_LOG2) - 1;
        }
        static inline size_t bucket_spacing_function_inverse(unsigned index)
        {
            return (size_t)(index + 1) << MIN_ALLOCATION_LOG2;
        }

        // block header is where the large HphaAllocatorPimpl stores its book-keeping information
        // it is always located in front of the payload block
        class block_header
        {
            enum block_flags : uint64_t
            {
                BL_USED = 1,
                BL_FLAG_MASK = 0x3
            };
            block_header* mPrev;
            // 16 bits of tag, 46 bits of size, 2 bits of flags (used or not)
            uint64_t mSizeAndFlags;

        public:
            using block_ptr = block_header*;
            size_t size() const
            {
                return mSizeAndFlags & ~BL_FLAG_MASK;
            }
            block_ptr next() const
            {
                return (block_ptr)((char*)mem() + size());
            }
            block_ptr prev() const
            {
                return mPrev;
            }
            void* mem() const
            {
                return (void*)((char*)this + sizeof(block_header));
            }
            bool used() const
            {
                return (mSizeAndFlags & BL_USED) != 0;
            }
            void set_used()
            {
                mSizeAndFlags |= BL_USED;
            }
            void set_unused()
            {
                mSizeAndFlags &= ~BL_USED;
            }
            void unlink()
            {
                next()->prev(prev());
                prev()->next(next());
            }
            void link_after(block_ptr link)
            {
                prev(link);
                next(link->next());
                next()->prev(this);
                prev()->next(this);
            }
            void size(size_t size)
            {
                HPPA_ASSERT((size & BL_FLAG_MASK) == 0);
                mSizeAndFlags = (mSizeAndFlags & BL_FLAG_MASK) | size;
            }
            void next(block_ptr next)
            {
                HPPA_ASSERT(next >= mem());
                size((char*)next - (char*)mem());
            }
            void prev(block_ptr prev)
            {
                mPrev = prev;
            }
        };

        struct block_header_proxy
        {
            unsigned char _proxy_size[sizeof(block_header)];
        };

        // the page structure is where the small HphaAllocatorPimpl stores all its book-keeping information
        // it is always located at the front of a OS page
        struct free_link
        {
            free_link* mNext;
        };
        struct page
            : public block_header_proxy     /* must be first */
            , public AZStd::list_base_hook<page>::node_type
        {
            page(size_t elemSize, size_t pageSize, size_t marker)
                : mBucketIndex((unsigned short)bucket_spacing_function_aligned(elemSize))
                , mUseCount(0)
            {
                mMarker = marker ^ ((size_t)this);

                // build the free list inside the new page
                // the page info sits at the front of the page
                size_t numElements = (pageSize - sizeof(page)) / elemSize;
                char* endMem = (char*)this + pageSize;
                char* currentMem = endMem - numElements * elemSize;
                char* nextMem = currentMem + elemSize;
                mFreeList = (free_link*)currentMem;
                while (nextMem != endMem)
                {
                    ((free_link*)currentMem)->mNext = (free_link*)(nextMem);
                    currentMem = nextMem;
                    nextMem += elemSize;
                }
                ((free_link*)currentMem)->mNext = nullptr;
            }

            inline void setInvalid()
            {
                mMarker = 0;
            }

            free_link* mFreeList = nullptr;
            size_t mMarker = 0;
            unsigned short mBucketIndex = 0;
            unsigned short mUseCount = 0;

            size_t elem_size() const
            {
                return bucket_spacing_function_inverse(mBucketIndex);
            }
            unsigned bucket_index() const
            {
                return mBucketIndex;
            }
            size_t count() const
            {
                return mUseCount;
            }
            bool empty() const
            {
                return mUseCount == 0;
            }
            void inc_ref()
            {
                mUseCount++;
            }
            void dec_ref()
            {
                HPPA_ASSERT(mUseCount > 0);
                mUseCount--;
            }
            bool check_marker(size_t marker) const
            {
                return mMarker == (marker ^ ((size_t)this));
            }
        };
        using page_list = AZStd::intrusive_list<page, AZStd::list_base_hook<page>>;

#if defined(MULTITHREADED) && defined(USE_MUTEX_PER_BUCKET)
        static constexpr size_t BucketAlignment = HphaInternal::AlignUpToPowerOfTwo(sizeof(page_list) + sizeof(AZStd::mutex) + sizeof(size_t));
#else
        static constexpr size_t BucketAlignment = HphaInternal::AlignUpToPowerOfTwo(sizeof(page_list) + sizeof(size_t));
#endif
        AZ_PUSH_DISABLE_WARNING_MSVC(4324)
        class alignas(BucketAlignment) bucket
        {
            page_list mPageList;
#if defined(MULTITHREADED) && defined(USE_MUTEX_PER_BUCKET)
            mutable AZStd::mutex mLock;
#endif
            size_t          mMarker;

        public:
            bucket();
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
            AZStd::mutex& get_lock() const
            {
                return mLock;
            }
#endif
#endif
            size_t marker() const
            {
                return mMarker;
            }
            auto page_list_begin() const
            {
                return mPageList.begin();
            }
            auto page_list_begin()
            {
                return mPageList.begin();
            }
            auto page_list_end() const
            {
                return mPageList.end();
            }
            auto page_list_end()
            {
                return mPageList.end();
            }
            bool page_list_empty() const
            {
                return mPageList.empty();
            }
            void add_free_page(page* p)
            {
                mPageList.push_front(*p);
            }
            page* get_free_page();
            const page* get_free_page() const;
            void* alloc(page* p);
            void free(page* p, void* ptr);
            void unlink(page* p);
        };
        AZ_POP_DISABLE_WARNING_MSVC
        void* bucket_system_alloc();
        void bucket_system_free(void* ptr);
        page* bucket_grow(size_t elemSize, size_t marker);
        void* bucket_alloc(size_t size);
        void* bucket_alloc_direct(unsigned bi);
        void* bucket_realloc(void* ptr, size_t size);
        void* bucket_realloc_aligned(void* ptr, size_t size, size_t alignment);
        void bucket_free(void* ptr);
        void bucket_free_direct(void* ptr, unsigned bi);
        /// return the block size for the pointer.
        size_t bucket_ptr_size(void* ptr) const;
        size_t bucket_get_max_allocation() const;
        size_t bucket_get_unused_memory(bool isPrint) const;
        void bucket_purge();

        // locate the page information from a pointer
        inline page* ptr_get_page(void* ptr) const
        {
            return (page*)AZ::PointerAlignDown((char*)ptr, m_poolPageSize);
        }

        bool ptr_in_bucket(void* ptr) const
        {
            bool result = false;
            page* p = ptr_get_page(ptr);
            unsigned bi = p->bucket_index();
            if (bi < NUM_BUCKETS)
            {
                result = p->check_marker(mBuckets[bi].marker());
                // there's a minimal chance the marker check is not sufficient
                // due to random data that happens to match the marker
            }
            return result;
        }

        // free node is what the large HphaAllocatorPimpl uses to find free space
        // it's stored next to the block header when a block is not in use
        struct free_node : public AZStd::intrusive_multiset_node<free_node>
        {
            block_header* get_block() const
            {
                return (block_header*)((char*)this - sizeof(block_header));
            }
            operator size_t() const
            {
                return get_block()->size();
            }

            friend bool operator<(const free_node& lhs, const free_node& rhs)
            {
                return static_cast<size_t>(lhs) < static_cast<size_t>(rhs);
            }
            friend bool operator<(const free_node& lhs, size_t rhs)
            {
                return static_cast<size_t>(lhs) < rhs;
            }
            friend bool operator<(size_t rhs, const free_node& lhs)
            {
                return rhs < static_cast<size_t>(lhs);
            }
        };

        using free_node_tree = AZStd::intrusive_multiset<free_node, AZStd::intrusive_multiset_base_hook<free_node>, AZStd::less<>>;

        static inline block_header* ptr_get_block_header(void* ptr)
        {
            return (block_header*)((char*)ptr - sizeof(block_header));
        }

        // helper functions to manipulate block headers
        void split_block(block_header* bl, size_t size);
        block_header* shift_block(block_header* bl, size_t offs);
        block_header* coalesce_block(block_header* bl);

        void* tree_system_alloc(size_t size);
        void tree_system_free(void* ptr, size_t size);
        block_header* tree_extract(size_t size);
        block_header* tree_extract_aligned(size_t size, size_t alignment);
        block_header* tree_extract_bucket_page();
        block_header* tree_add_block(void* mem, size_t size);
        block_header* tree_grow(size_t size);
        void tree_attach(block_header* bl);
        void tree_detach(block_header* bl);
        void tree_purge_block(block_header* bl);

        void* tree_alloc(size_t size);
        void* tree_alloc_aligned(size_t size, size_t alignment);
        void* tree_alloc_bucket_page();
        void* tree_realloc(void* ptr, size_t size);
        void* tree_realloc_aligned(void* ptr, size_t size, size_t alignment);
        size_t tree_resize(void* ptr, size_t size);
        void tree_free(void* ptr);
        void tree_free_bucket_page(
            void* ptr); // only allocations with tree_alloc_bucket_page should be passed here, we don't have any guards.
        size_t tree_ptr_size(void* ptr) const;
        size_t tree_get_max_allocation() const;
        size_t tree_get_unused_memory(bool isPrint) const;
        void tree_purge();

        enum debug_source
        {
            DEBUG_SOURCE_BUCKETS = 0,
            DEBUG_SOURCE_TREE = 1,
            DEBUG_SOURCE_SIZE = 2,
            DEBUG_SOURCE_INVALID = DEBUG_SOURCE_SIZE
        };
        enum memory_debugging_flags
        {
            DEBUG_FLAGS_NONE = 0,
            DEBUG_FLAGS_FILLING = 1,
            DEBUG_FLAGS_GUARD_CHECK = 2,

            DEBUG_FLAGS_ALL = DEBUG_FLAGS_FILLING | DEBUG_FLAGS_GUARD_CHECK
        };
        static constexpr size_t DEBUG_UNKNOWN_SIZE = (size_t)-1;

        // allocate memory using DEFAULT_ALIGNMENT
        // size == 0 returns NULL
        inline void* alloc(size_t size)
        {
            if (size == 0)
            {
                return nullptr;
            }
            if (is_small_allocation(size))
            {
                size = clamp_small_allocation(size);
                void* ptr = bucket_alloc_direct(bucket_spacing_function(size));
                return ptr;
            }
            else
            {
                void* ptr = tree_alloc(size);
                return ptr;
            }
        }

        // allocate memory with a specific alignment
        // size == 0 returns NULL
        // alignment <= DEFAULT_ALIGNMENT acts as alloc
        inline void* alloc(size_t size, size_t alignment)
        {
            HPPA_ASSERT((alignment & (alignment - 1)) == 0);
            if (alignment <= DEFAULT_ALIGNMENT)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                return nullptr;
            }
            if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
            {
                size = clamp_small_allocation(size);
                void* ptr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size, alignment)));
                return ptr;
            }
            else
            {
                void* ptr = tree_alloc_aligned(size, alignment);
                return ptr;
            }
        }

        // realloc the memory block using DEFAULT_ALIGNMENT
        // ptr == NULL acts as alloc
        // size == 0 acts as free
        void* realloc(void* ptr, size_t size)
        {
            if (ptr == nullptr)
            {
                return alloc(size);
            }
            if (size == 0)
            {
                free(ptr);
                return nullptr;
            }
            void* newPtr = nullptr;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size))
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_realloc(ptr, size);
                }
                else
                {
                    newPtr = tree_alloc(size);
                    if (newPtr)
                    {
                        HPPA_ASSERT(size >= ptr_get_page(ptr)->elem_size());
                        memcpy(newPtr, ptr, ptr_get_page(ptr)->elem_size());
                        bucket_free(ptr);
                    }
                }
            }
            else
            {
                if (is_small_allocation(size))
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_alloc(size);
                    if (newPtr)
                    {
                        HPPA_ASSERT(size <= ptr_get_block_header(ptr)->size());
                        memcpy(newPtr, ptr, size);
                        tree_free(ptr);
                    }
                }
                else
                {
                    newPtr = tree_realloc(ptr, size);
                }
            }
            return newPtr;
        }

        // realloc the memory block using a specific alignment
        // ptr == NULL acts as alloc
        // size == 0 acts as free
        // alignment <= DEFAULT_ALIGNMENT acts as realloc
        void* realloc(void* ptr, size_t size, size_t alignment)
        {
            HPPA_ASSERT((alignment & (alignment - 1)) == 0);
            if (alignment <= DEFAULT_ALIGNMENT)
            {
                return realloc(ptr, size);
            }
            if (ptr == nullptr)
            {
                return alloc(size, alignment);
            }
            if (size == 0)
            {
                free(ptr);
                return nullptr;
            }
            if ((size_t)ptr & (alignment - 1))
            {
                void* newPtr = alloc(size, alignment);
                if (!newPtr)
                {
                    return nullptr;
                }
                size_t count = this->size(ptr);
                if (count > size)
                {
                    count = size;
                }
                memcpy(newPtr, ptr, count);
                free(ptr);
                return newPtr;
            }
            void* newPtr = nullptr;
            if (ptr_in_bucket(ptr))
            {
                if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION) 
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_realloc_aligned(ptr, size, alignment);
                }
                else
                {
                    newPtr = tree_alloc_aligned(size, alignment);
                    if (newPtr)
                    {
                        HPPA_ASSERT(size >= ptr_get_page(ptr)->elem_size());
                        memcpy(newPtr, ptr, ptr_get_page(ptr)->elem_size());
                        bucket_free(ptr);
                    }
                }
            }
            else
            {
                if (is_small_allocation(size) && alignment <= MAX_SMALL_ALLOCATION)
                {
                    size = clamp_small_allocation(size);
                    newPtr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size, alignment)));
                    if (newPtr)
                    {
                        HPPA_ASSERT(size <= ptr_get_block_header(ptr)->size());
                        memcpy(newPtr, ptr, size);
                        tree_free(ptr);
                    }
                }
                else
                {
                    newPtr = tree_realloc_aligned(ptr, size, alignment);
                }
            }
            return newPtr;
        }

        // resize the memory block to the extent possible
        // returns the size of the resulting memory block
        inline size_t resize(void* ptr, size_t size)
        {
            if (ptr == nullptr)
            {
                return 0;
            }
            HPPA_ASSERT(size > 0);
            if (ptr_in_bucket(ptr))
            {
                size = ptr_get_page(ptr)->elem_size();
                return size;
            }

            // from the tree we should allocate only >= MAX_SMALL_ALLOCATION
            size = AZStd::GetMax(size, MAX_SMALL_ALLOCATION + MIN_ALLOCATION);

            size = tree_resize(ptr, size);
            return size;
        }

        // query the size of the memory block
        inline size_t size(void* ptr) const
        {
            if (ptr == nullptr)
            {
                return 0;
            }
            if (ptr_in_bucket(ptr))
            {
                return ptr_get_page(ptr)->elem_size();
            }
            block_header* bl = ptr_get_block_header(ptr);
            size_t blockSize = bl->size();
            return blockSize;
        }

        // free the memory block
        inline void free(void* ptr)
        {
            if (ptr == nullptr)
            {
                return;
            }
            if (ptr_in_bucket(ptr))
            {
                return bucket_free(ptr);
            }
            tree_free(ptr);
        }

        // free the memory block supplying the original size with DEFAULT_ALIGNMENT
        inline void free(void* ptr, size_t origSize)
        {
            if (ptr == nullptr)
            {
                return;
            }
            if (is_small_allocation(origSize))
            {
                // if this asserts probably the original alloc used alignment
                HPPA_ASSERT(ptr_in_bucket(ptr));
                return bucket_free_direct(ptr, bucket_spacing_function(origSize));
            }
            tree_free(ptr);
        }

        // free the memory block supplying the original size and alignment
        inline void free(void* ptr, size_t origSize, size_t oldAlignment)
        {
            if (ptr == nullptr)
            {
                return;
            }
            if (is_small_allocation(origSize) && oldAlignment <= MAX_SMALL_ALLOCATION)
            {
                HPPA_ASSERT(ptr_in_bucket(ptr), "small object ptr not in a bucket");
                return bucket_free_direct(ptr, bucket_spacing_function(AZ::SizeAlignUp(origSize, oldAlignment)));
            }
            tree_free(ptr);
        }

        // return all unused memory pages to the OS
        // this function doesn't need to be called unless needed
        // it can be called periodically when large amounts of memory can be reclaimed
        // in all cases memory is never automatically returned to the OS
        void purge()
        {
            // Purge buckets first since they use tree pages
            bucket_purge();
            tree_purge();
        }

    public:
        HphaAllocatorPimpl(IAllocator& subAllocator);
        ~HphaAllocatorPimpl() override;

        pointer allocate(size_type byteSize, align_type alignment = 1) override;
        void deallocate(pointer ptr, size_type byteSize = 0, align_type alignment = 0) override;
        pointer reallocate(pointer ptr, size_type newSize, align_type alignment = 1) override;
        size_type get_allocated_size(pointer ptr, align_type alignment = 1) const override;

        void Merge(IAllocator* aOther) override;

        void GarbageCollect() override;

    private:
        AZ_DISABLE_COPY_MOVE(HphaAllocatorPimpl)

        AZ_PUSH_DISABLE_WARNING_MSVC(4324)
        bucket mBuckets[NUM_BUCKETS];
        AZ_POP_DISABLE_WARNING_MSVC
#if defined(MULTITHREADED) && !defined(USE_MUTEX_PER_BUCKET)
        mutable AZStd::mutex m_bucketsMutex;
#endif

        free_node_tree mFreeTree;
#ifdef MULTITHREADED
        // TODO rbbaklov: switched to recursive_mutex from mutex for Linux support.
        mutable AZStd::recursive_mutex mTreeMutex;
#endif

        IAllocator& m_subAllocator;

        const size_t m_treePageSize;
        const size_t m_treePageAlignment;
        const size_t m_poolPageSize;
    };

#ifdef DEBUG_MULTI_RBTREE
    unsigned intrusive_multi_rbtree_base::check_height(node_base* node) const
    {
        if (node == &mHead)
        {
            return 0;
        }
        if (node->black())
        {
            return check_height(node->child(LEFT)) + check_height(node->child(RIGHT)) + 1;
        }
        HPPA_ASSERT(node->child(LEFT)->black() && node->child(RIGHT)->black());
        unsigned lh = check_height(node->child(LEFT));
        unsigned rh = check_height(node->child(RIGHT));
        HPPA_ASSERT(lh == rh);
        return lh;
    }

    void intrusive_multi_rbtree_base::dump(node_base* node) const
    {
        if (node == nullptr)
        {
            node = mHead.child(LEFT);
        }
        if (!node->nil())
        {
            AZ_TracePrintf(
                "HphaAllocator", "%p(%s|%s|%p|%p|%p)\n", node, node->red() ? "red" : "black", node->parent_side() == LEFT ? "L" : "R",
                reinterpret_cast<void*>(reinterpret_cast<int>(node->parent()) & ~3), node->left()->nil() ? nullptr : node->left(),
                node->right()->nil() ? nullptr : node->right());
            dump(node->left());
            dump(node->right());
        }
    }

    void intrusive_multi_rbtree_base::check() const
    {
        HPPA_ASSERT(mHead.black());
        HPPA_ASSERT(mHead.child(RIGHT) == &mHead);
        HPPA_ASSERT(mHead.child(LEFT) == &mHead || mHead.child(LEFT)->black());
        check_height(mHead.child(LEFT));
    }
#endif

    //////////////////////////////////////////////////////////////////////////
    HphaAllocatorPimpl::HphaAllocatorPimpl(IAllocator& subAllocator)
        // m_treePageAlignment should be OS_VIRTUAL_PAGE_SIZE in all cases with this trait as we work
        // with virtual memory addresses when the tree grows and we cannot specify an alignment in all cases
        : m_treePageSize(OS_VIRTUAL_PAGE_SIZE)
        , m_treePageAlignment(OS_VIRTUAL_PAGE_SIZE)
        , m_poolPageSize(OS_VIRTUAL_PAGE_SIZE)
        , m_subAllocator(subAllocator)
    {
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#if defined(MULTITHREADED)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount((LPCRITICAL_SECTION)mTreeMutex.native_handle(), SPIN_COUNT);
#endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT

#if !defined(USE_MUTEX_PER_BUCKET)
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#if defined(MULTITHREADED)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount(m_bucketsMutex.native_handle(), SPIN_COUNT);
#endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#endif
    }

    HphaAllocatorPimpl::~HphaAllocatorPimpl()
    {
        purge();
    }

    HphaAllocatorPimpl::bucket::bucket()
    {
#if AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT
#if defined(MULTITHREADED)
#if defined(USE_MUTEX_PER_BUCKET)
        // For some platforms we can use an actual spin lock, test and profile. We don't expect much contention there
        SetCriticalSectionSpinCount((LPCRITICAL_SECTION)mLock.native_handle(), SPIN_COUNT);
#endif
#endif // MULTITHREADED
#endif // AZ_TRAIT_OS_HAS_CRITICAL_SECTION_SPIN_COUNT

        // Initializing Sfmt causes a file read which causes an allocation which prevents us from initializing the allocators before we need
        // malloc overridden. Thats why we use SimpleLcgRandom here
        AZ::SimpleLcgRandom randGenerator = AZ::SimpleLcgRandom(reinterpret_cast<u64>(static_cast<void*>(this)));
        mMarker = size_t(randGenerator.Getu64Random());
    }

    HphaAllocatorPimpl::page* HphaAllocatorPimpl::bucket::get_free_page()
    {
        if (!mPageList.empty())
        {
            page* p = &mPageList.front();
            if (p->mFreeList)
            {
                return p;
            }
        }
        return nullptr;
    }

    const HphaAllocatorPimpl::page* HphaAllocatorPimpl::bucket::get_free_page() const
    {
        if (!mPageList.empty())
        {
            const page* p = &mPageList.front();
            if (p->mFreeList)
            {
                return p;
            }
        }
        return nullptr;
    }

    void* HphaAllocatorPimpl::bucket::alloc(page* p)
    {
        // get an element from the free list
        HPPA_ASSERT(p && p->mFreeList);
        p->inc_ref();
        free_link* free = p->mFreeList;
        free_link* next = free->mNext;
        p->mFreeList = next;
        if (!next)
        {
            // if full, auto sort to back
            mPageList.erase(*p);
            mPageList.push_back(*p);
        }
        return (void*)free;
    }

    void HphaAllocatorPimpl::bucket::free(page* p, void* ptr)
    {
        // add the element back to the free list
        free_link* free = p->mFreeList;
        free_link* lnk = (free_link*)ptr;
        lnk->mNext = free;
        p->mFreeList = lnk;
        p->dec_ref();
        if (!free)
        {
            // if the page was previously full, auto sort to front
            mPageList.erase(*p);
            mPageList.push_front(*p);
        }
    }

    void HphaAllocatorPimpl::bucket::unlink(page* p)
    {
        mPageList.erase(*p);
    }

    void* HphaAllocatorPimpl::bucket_system_alloc()
    {
        void* ptr = m_subAllocator.allocate(m_poolPageSize, m_poolPageSize);
#if defined(AZ_ENABLE_TRACING)
        if (ptr)
        {
            AddAllocated(m_poolPageSize);
        }
#endif
        HPPA_ASSERT(((size_t)ptr & (m_poolPageSize - 1)) == 0); // checks alignment
        return ptr;
    }

    void HphaAllocatorPimpl::bucket_system_free(void* ptr)
    {
        HPPA_ASSERT(ptr);
#if defined(AZ_ENABLE_TRACING)
        RemoveAllocated(m_poolPageSize);
#endif
        m_subAllocator.deallocate(ptr, m_poolPageSize, m_poolPageSize);
    }

    HphaAllocatorPimpl::page* HphaAllocatorPimpl::bucket_grow(size_t elemSize, size_t marker)
    {
        // make sure mUseCount won't overflow
        HPPA_ASSERT((m_poolPageSize - sizeof(page)) / elemSize <= USHRT_MAX);
        if (void* mem = bucket_system_alloc())
        {
            return new (mem) page((unsigned short)elemSize, m_poolPageSize, marker);
        }
        return nullptr;
    }

    void* HphaAllocatorPimpl::bucket_alloc(size_t size)
    {
        HPPA_ASSERT(size <= MAX_SMALL_ALLOCATION);
        unsigned bi = bucket_spacing_function(size);
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
        // get the page info and check if there's any available elements
        page* p = mBuckets[bi].get_free_page();
        if (!p)
        {
            // get a page from the OS, initialize it and add it to the list
            size_t bsize = bucket_spacing_function_inverse(bi);
            p = bucket_grow(bsize, mBuckets[bi].marker());
            if (!p)
            {
                return nullptr;
            }
            mBuckets[bi].add_free_page(p);
        }
        HPPA_ASSERT(p->elem_size() >= size);
        return mBuckets[bi].alloc(p);
    }

    void* HphaAllocatorPimpl::bucket_alloc_direct(unsigned bi)
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
        page* p = mBuckets[bi].get_free_page();
        if (!p)
        {
            size_t bsize = bucket_spacing_function_inverse(bi);
            p = bucket_grow(bsize, mBuckets[bi].marker());
            if (!p)
            {
                return nullptr;
            }
            mBuckets[bi].add_free_page(p);
        }
        return mBuckets[bi].alloc(p);
    }

    void* HphaAllocatorPimpl::bucket_realloc(void* ptr, size_t size)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        // if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        // if (size <= elemSize)
        if (size == elemSize)
        {
            return ptr;
        }
        void* newPtr = bucket_alloc(size);
        if (!newPtr)
        {
            return nullptr;
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize, size));
        bucket_free(ptr);
        return newPtr;
    }

    void* HphaAllocatorPimpl::bucket_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        page* p = ptr_get_page(ptr);
        size_t elemSize = p->elem_size();
        // if we do this and we use the bucket_free with size hint we will crash, as the hit will not be the real index/bucket size
        // if (size <= elemSize && (elemSize&(alignment-1))==0)
        if (size == elemSize && (elemSize & (alignment - 1)) == 0)
        {
            return ptr;
        }
        void* newPtr = bucket_alloc_direct(bucket_spacing_function(AZ::SizeAlignUp(size, alignment)));
        if (!newPtr)
        {
            return nullptr;
        }
        memcpy(newPtr, ptr, AZStd::GetMin(elemSize, size));
        bucket_free(ptr);
        return newPtr;
    }

    void HphaAllocatorPimpl::bucket_free(void* ptr)
    {
        page* p = ptr_get_page(ptr);
        unsigned bi = p->bucket_index();
        HPPA_ASSERT(bi < NUM_BUCKETS);
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
        mBuckets[bi].free(p, ptr);
    }

    void HphaAllocatorPimpl::bucket_free_direct(void* ptr, unsigned bi)
    {
        HPPA_ASSERT(bi < NUM_BUCKETS);
        page* p = ptr_get_page(ptr);
        // if this asserts, the free size doesn't match the allocated size
        // most likely a class needs a base virtual destructor
        HPPA_ASSERT(bi == p->bucket_index());
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
        mBuckets[bi].free(p, ptr);
    }

    size_t HphaAllocatorPimpl::bucket_ptr_size(void* ptr) const
    {
        page* p = ptr_get_page(ptr);
        HPPA_ASSERT(p->bucket_index() < NUM_BUCKETS);

#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
        unsigned bi = p->bucket_index();
        AZStd::lock_guard<AZStd::mutex> lock(mBuckets[bi].get_lock());
#else
        AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
        return p->elem_size();
    }

    size_t HphaAllocatorPimpl::bucket_get_max_allocation() const
    {
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#else
            AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
            const page* p = mBuckets[i].get_free_page();
            if (p)
            {
                return p->elem_size();
            }
        }

        return 0;
    }

    size_t HphaAllocatorPimpl::bucket_get_unused_memory(bool isPrint) const
    {
        size_t unusedMemory = 0;
        size_t availablePageMemory = m_poolPageSize - sizeof(page);
        for (int i = (int)NUM_BUCKETS - 1; i > 0; i--)
        {
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#else
            AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
            auto pageEnd = mBuckets[i].page_list_end();
            for (auto p = mBuckets[i].page_list_begin(); p != pageEnd;)
            {
                // early out if we reach fully occupied page (the remaining should all be full)
                if (p->mFreeList == nullptr)
                {
                    break;
                }
                size_t elementSize = p->elem_size();
                size_t availableMemory = availablePageMemory - (elementSize * p->count());
                unusedMemory += availableMemory;
                if (isPrint)
                {
                    AZ_TracePrintf(
                        "System", "Unused Bucket %d page %p elementSize: %d available: %d elements\n", i, p, elementSize,
                        availableMemory / elementSize);
                }
                p = p->m_next;
            }
        }
        return unusedMemory;
    }

    void HphaAllocatorPimpl::bucket_purge()
    {
        for (unsigned i = 0; i < NUM_BUCKETS; i++)
        {
#ifdef MULTITHREADED
#if defined(USE_MUTEX_PER_BUCKET)
            AZStd::lock_guard<AZStd::mutex> lock(mBuckets[i].get_lock());
#else
            AZStd::lock_guard<AZStd::mutex> lock(m_bucketsMutex);
#endif
#endif
            auto pageEnd = mBuckets[i].page_list_end();
            for (auto p = mBuckets[i].page_list_begin(); p != pageEnd;)
            {
                // early out if we reach fully occupied page (the remaining should all be full)
                if (p->mFreeList == nullptr)
                {
                    break;
                }
                page* next = p->m_next;
                if (p->empty())
                {
                    HPPA_ASSERT(p->mFreeList);
                    mBuckets[i].unlink(AZStd::to_address(p));
                    p->setInvalid();
                    bucket_system_free(AZStd::to_address(p));
                }
                p = next;
            }
        }
    }

    void HphaAllocatorPimpl::split_block(block_header* bl, size_t size)
    {
        HPPA_ASSERT(size + sizeof(block_header) + sizeof(free_node) <= bl->size());
        block_header* newBl = (block_header*)((char*)bl + size + sizeof(block_header));
        newBl->link_after(bl);
        newBl->set_unused();
    }

    HphaAllocatorPimpl::block_header* HphaAllocatorPimpl::shift_block(block_header* bl, size_t offs)
    {
        HPPA_ASSERT(offs > 0);
        block_header* prev = bl->prev();
        bl->unlink();
        bl = (block_header*)((char*)bl + offs);
        bl->link_after(prev);
        bl->set_unused();
        return bl;
    }

    HphaAllocatorPimpl::block_header* HphaAllocatorPimpl::coalesce_block(block_header* bl)
    {
        HPPA_ASSERT(!bl->used());
        block_header* next = bl->next();
        if (!next->used())
        {
            tree_detach(next);
            next->unlink();
        }
        block_header* prev = bl->prev();
        if (!prev->used())
        {
            tree_detach(prev);
            bl->unlink();
            bl = prev;
        }
        return bl;
    }

    void* HphaAllocatorPimpl::tree_system_alloc(size_t size)
    {
        const size_t allocSize = AZ::SizeAlignUp(size, OS_VIRTUAL_PAGE_SIZE);
        void* ptr = m_subAllocator.allocate(allocSize, m_treePageAlignment);
#if defined(AZ_ENABLE_TRACING)
        if (ptr)
        {
            AddAllocated(allocSize);
        }
#endif
        return ptr;
    }

    void HphaAllocatorPimpl::tree_system_free(void* ptr, size_t size)
    {
        HPPA_ASSERT(ptr);

        const size_t allocSize = AZ::SizeAlignUp(size, OS_VIRTUAL_PAGE_SIZE);
        m_subAllocator.deallocate(ptr, allocSize, m_treePageAlignment);
#if defined(AZ_ENABLE_TRACING)
        RemoveAllocated(allocSize);
#endif
    }

    HphaAllocatorPimpl::block_header* HphaAllocatorPimpl::tree_add_block(void* mem, size_t size)
    {
        // create a dummy block to avoid prev() NULL checks and allow easy block shifts
        // potentially this dummy block might grow (due to shift_block) but not more than sizeof(free_node)
        block_header* front = (block_header*)mem;
        front->prev(nullptr);
        front->size(0);
        front->set_used();
        block_header* back = (block_header*)front->mem();
        back->prev(front);
        back->size(0);
        // now the real free block
        front = back;
        back = (block_header*)((char*)mem + size - sizeof(block_header));
        back->size(0);
        back->set_used();
        front->set_unused();
        front->next(back);
        back->prev(front);
        return front;
    }

    HphaAllocatorPimpl::block_header* HphaAllocatorPimpl::tree_grow(size_t size)
    {
        const size_t sizeWithBlockHeaders = size + 3 * sizeof(block_header); // two fences plus one fake
        const size_t newSize =
            (sizeWithBlockHeaders < m_treePageSize) ? AZ::SizeAlignUp(sizeWithBlockHeaders, m_treePageSize) : sizeWithBlockHeaders;
        HPPA_ASSERT(newSize >= sizeWithBlockHeaders);

        if (void* mem = tree_system_alloc(newSize))
        {
            return tree_add_block(mem, newSize);
        }
        return nullptr;
    }

    HphaAllocatorPimpl::block_header* HphaAllocatorPimpl::tree_extract(size_t size)
    {
        // search the tree and get the smallest fitting block
        free_node_tree::iterator it = mFreeTree.lower_bound(size);
        if (it == mFreeTree.end())
        {
            return nullptr;
        }
        free_node* bestNode = it->next(); // improves removal time
        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    HphaAllocatorPimpl::block_header* HphaAllocatorPimpl::tree_extract_aligned(size_t size, size_t alignment)
    {
        // get the sequence of nodes from size to (size + alignment - 1) including
        size_t sizeUpper = size + alignment;
        free_node_tree::iterator bestNode = mFreeTree.lower_bound(size);
        free_node_tree::iterator lastNode = mFreeTree.upper_bound(sizeUpper);
        while (bestNode != lastNode)
        {
            free_node* node = &*bestNode;
            size_t alignmentOffs = AZ::PointerAlignUp((char*)node, alignment) - (char*)node;
            if (node->get_block()->size() >= size + alignmentOffs)
            {
                break;
            }
            // keep walking the sequence till we find a match or reach the end
            // the larger the alignment the more linear searching will be done
            ++bestNode;
        }
        if (bestNode == mFreeTree.end())
        {
            return nullptr;
        }
        if (bestNode == lastNode)
        {
            bestNode = bestNode->next(); // improves removal time
        }
        block_header* bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    HphaAllocatorPimpl::block_header* HphaAllocatorPimpl::tree_extract_bucket_page()
    {
        block_header* bestBlock = nullptr;
        size_t alignment = m_poolPageSize;
        size_t size = m_poolPageSize;

        // get the sequence of nodes from size to (size + alignment - 1) including
        size_t sizeUpper = size + alignment;
        free_node_tree::iterator bestNode = mFreeTree.lower_bound(size);
        free_node_tree::iterator lastNode = mFreeTree.upper_bound(sizeUpper);
        while (bestNode != lastNode)
        {
            bestBlock = bestNode->get_block();
            size_t alignmentOffs = AZ::PointerAlignUp((char*)bestBlock, alignment) - (char*)bestBlock;
            if (bestBlock->size() + sizeof(block_header) >= size + alignmentOffs)
            {
                break;
            }
            // keep walking the sequence till we find a match or reach the end
            // the larger the alignment the more linear searching will be done
            ++bestNode;
        }
        if (bestNode == mFreeTree.end())
        {
            return nullptr;
        }
        if (bestNode == lastNode)
        {
            bestNode = bestNode->next(); // improves removal time
        }
        bestBlock = bestNode->get_block();
        tree_detach(bestBlock);
        return bestBlock;
    }

    void HphaAllocatorPimpl::tree_attach(block_header* bl)
    {
        mFreeTree.insert((free_node*)bl->mem());
    }

    void HphaAllocatorPimpl::tree_detach(block_header* bl)
    {
        mFreeTree.erase((free_node*)bl->mem());
    }

    void* HphaAllocatorPimpl::tree_alloc(size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        // modify the size to make sure we can fit the block header and free node
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        // extract a block from the tree if found
        block_header* newBl = tree_extract(size);
        if (!newBl)
        {
            // ask the OS for more memory
            newBl = tree_grow(size);
            if (!newBl)
            {
                return nullptr;
            }
        }
        HPPA_ASSERT(!newBl->used());
        HPPA_ASSERT(newBl && newBl->size() >= size);
        // chop off any remainder
        if (newBl->size() >= size + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size);
            tree_attach(newBl->next());
        }
        newBl->set_used();
        return newBl->mem();
    }

    void* HphaAllocatorPimpl::tree_alloc_aligned(size_t size, size_t alignment)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* newBl = tree_extract_aligned(size, alignment);
        if (!newBl)
        {
            newBl = tree_grow(size + alignment);
            if (!newBl)
            {
                return nullptr;
            }
        }
        HPPA_ASSERT(newBl && newBl->size() >= size);
        size_t alignmentOffs = AZ::PointerAlignUp((char*)newBl->mem(), alignment) - (char*)newBl->mem();
        HPPA_ASSERT(newBl->size() >= size + alignmentOffs);
        if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, alignmentOffs - sizeof(block_header));
            tree_attach(newBl);
            newBl = newBl->next();
        }
        else if (alignmentOffs > 0)
        {
            newBl = shift_block(newBl, alignmentOffs);
        }
        if (newBl->size() >= size + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size);
            tree_attach(newBl->next());
        }
        newBl->set_used();
        HPPA_ASSERT(((size_t)newBl->mem() & (alignment - 1)) == 0);
        return newBl->mem();
    }

    void* HphaAllocatorPimpl::tree_alloc_bucket_page()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        // We are allocating pool pages m_poolPageSize aligned at m_poolPageSize
        // what is special is that we are keeping the block_header at the beginning of the
        // memory and we return the offset pointer.
        size_t size = m_poolPageSize;
        size_t alignment = m_poolPageSize;
        block_header* newBl = tree_extract_bucket_page();
        if (!newBl)
        {
            newBl = tree_grow(size + alignment);
            if (!newBl)
            {
                return nullptr;
            }
        }
        HPPA_ASSERT(newBl && (newBl->size() + sizeof(block_header)) >= size);
        size_t alignmentOffs = AZ::PointerAlignUp((char*)newBl, alignment) - (char*)newBl;
        HPPA_ASSERT(newBl->size() + sizeof(block_header) >= size + alignmentOffs);
        if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, alignmentOffs - sizeof(block_header));
            tree_attach(newBl);
            newBl = newBl->next();
        }
        else if (alignmentOffs > 0)
        {
            newBl = shift_block(newBl, alignmentOffs);
        }
        if (newBl->size() >= (size - sizeof(block_header)) + sizeof(block_header) + sizeof(free_node))
        {
            split_block(newBl, size - sizeof(block_header));
            tree_attach(newBl->next());
        }
        newBl->set_used();
        HPPA_ASSERT(((size_t)newBl & (alignment - 1)) == 0);
        return newBl;
    }

    void* HphaAllocatorPimpl::tree_realloc(void* ptr, size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            // the block is being shrunk, truncate and mark the remainder as free
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            HPPA_ASSERT(bl->size() >= size);
            return ptr;
        }
        // check if the following block is free and if it can accommodate the requested size
        block_header* next = bl->next();
        size_t nextSize = next->used() ? 0 : next->size() + sizeof(block_header);
        if (blSize + nextSize >= size)
        {
            HPPA_ASSERT(!next->used());
            tree_detach(next);
            next->unlink();
            HPPA_ASSERT(bl->size() >= size);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return ptr;
        }
        // check if the previous block can be used to avoid searching
        block_header* prev = bl->prev();
        size_t prevSize = prev->used() ? 0 : prev->size() + sizeof(block_header);
        if (blSize + prevSize + nextSize >= size)
        {
            HPPA_ASSERT(!prev->used());
            tree_detach(prev);
            bl->unlink();
            if (!next->used())
            {
                tree_detach(next);
                next->unlink();
            }
            bl = prev;
            bl->set_used();
            HPPA_ASSERT(bl->size() >= size);
            void* newPtr = bl->mem();
            // move the memory before we split the block
            memmove(newPtr, ptr, blSize);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return newPtr;
        }
        // fall back to alloc/copy/free
        void* newPtr = tree_alloc(size);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize);
            tree_free(ptr);
            return newPtr;
        }
        return nullptr;
    }

    void* HphaAllocatorPimpl::tree_realloc_aligned(void* ptr, size_t size, size_t alignment)
    {
        HPPA_ASSERT(((size_t)ptr & (alignment - 1)) == 0);
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            HPPA_ASSERT(bl->size() >= size);
            return ptr;
        }
        block_header* next = bl->next();
        size_t nextSize = next->used() ? 0 : next->size() + sizeof(block_header);
        if (blSize + nextSize >= size)
        {
            HPPA_ASSERT(!next->used());
            tree_detach(next);
            next->unlink();
            HPPA_ASSERT(bl->size() >= size);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return ptr;
        }
        block_header* prev = bl->prev();
        size_t prevSize = prev->used() ? 0 : prev->size() + sizeof(block_header);
        size_t alignmentOffs = prev->used() ? 0 : AZ::PointerAlignUp((char*)prev->mem(), alignment) - (char*)prev->mem();
        if (blSize + prevSize + nextSize >= size + alignmentOffs)
        {
            HPPA_ASSERT(!prev->used());
            tree_detach(prev);
            bl->unlink();
            if (!next->used())
            {
                tree_detach(next);
                next->unlink();
            }
            if (alignmentOffs >= sizeof(block_header) + sizeof(free_node))
            {
                split_block(prev, alignmentOffs - sizeof(block_header));
                tree_attach(prev);
                prev = prev->next();
            }
            else if (alignmentOffs > 0)
            {
                prev = shift_block(prev, alignmentOffs);
            }
            bl = prev;
            bl->set_used();
            HPPA_ASSERT(bl->size() >= size && ((size_t)bl->mem() & (alignment - 1)) == 0);
            void* newPtr = bl->mem();
            memmove(newPtr, ptr, blSize);
            if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                tree_attach(bl->next());
            }
            return newPtr;
        }
        void* newPtr = tree_alloc_aligned(size, alignment);
        if (newPtr)
        {
            memcpy(newPtr, ptr, blSize);
            tree_free(ptr);
            return newPtr;
        }
        return nullptr;
    }

    size_t HphaAllocatorPimpl::tree_resize(void* ptr, size_t size)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        if (size < sizeof(free_node))
        {
            size = sizeof(free_node);
        }
        size = AZ::SizeAlignUp(size, sizeof(block_header));
        block_header* bl = ptr_get_block_header(ptr);
        size_t blSize = bl->size();
        if (blSize >= size)
        {
            if (blSize >= size + sizeof(block_header) + sizeof(free_node))
            {
                split_block(bl, size);
                block_header* next = bl->next();
                next = coalesce_block(next);
                tree_attach(next);
            }
            HPPA_ASSERT(bl->size() >= size);
        }
        else
        {
            block_header* next = bl->next();
            if (!next->used() && blSize + next->size() + sizeof(block_header) >= size)
            {
                tree_detach(next);
                next->unlink();
                if (bl->size() >= size + sizeof(block_header) + sizeof(free_node))
                {
                    split_block(bl, size);
                    tree_attach(bl->next());
                }
                HPPA_ASSERT(bl->size() >= size);
            }
        }
        return bl->size();
    }

    void HphaAllocatorPimpl::tree_free(void* ptr)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        block_header* bl = ptr_get_block_header(ptr);
        // This assert detects a double-free of ptr
        HPPA_ASSERT(bl->used());
        bl->set_unused();
        bl = coalesce_block(bl);
        tree_attach(bl);
    }

    void HphaAllocatorPimpl::tree_free_bucket_page(void* ptr)
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        HPPA_ASSERT(AZ::PointerAlignDown(ptr, m_poolPageSize) == ptr);
        block_header* bl = (block_header*)ptr;
        HPPA_ASSERT(bl->size() >= m_poolPageSize - sizeof(block_header));
        bl->set_unused();
        bl = coalesce_block(bl);
        tree_attach(bl);
    }

    void HphaAllocatorPimpl::tree_purge_block(block_header* bl)
    {
        HPPA_ASSERT(!bl->used());
        HPPA_ASSERT(bl->prev() && bl->prev()->used());
        HPPA_ASSERT(bl->next() && bl->next()->used());
        if (bl->prev()->prev() == nullptr && bl->next()->size() == 0)
        {
            tree_detach(bl);
            char* memStart = (char*)bl->prev();
            char* memEnd = (char*)bl->mem() + bl->size() + sizeof(block_header);
            void* mem = memStart;
            size_t size = memEnd - memStart;
            HPPA_ASSERT(((size_t)mem & (m_treePageAlignment - 1)) == 0);
            memset(mem, 0xff, sizeof(block_header));
            tree_system_free(mem, size);
        }
        //  else if( bl->next()->size() == 0)
        //  {
        //      char* alignedStart = AZ::PointerAlignUp((char*)bl+requiredEndSize,m_treePageSize);
        //      char* memEnd = (char*)bl->next()+sizeof(block_header);
        //      if( alignedStart < memEnd )
        //      {
        //          tree_detach(bl);
        //          block_header* backFence = (block_header*)(alignedStart-sizeof(block_header));
        //          block_header* backFreeBlock = bl;
        //          backFreeBlock->next(backFence); // set's the size
        //
        //          backFence->prev(backFreeBlock);
        //          backFence->size(0);
        //          backFence->set_used();

        //          tree_attach(backFreeBlock);
        //
        //          memset(alignedStart,0xff,sizeof(block_header));
        //          tree_system_free(alignedStart, memEnd - alignedStart);
        //      }
        //  }
        //  else
        //  {
        //      char* alignedStart = AZ::PointerAlignUp((char*)bl+requiredEndSize,m_treePageSize);
        //      char* alignedEnd = AZ::PointerAlignDown((char*)bl->next()-requiredFrontSize,m_treePageSize);
        //      if( alignedStart < alignedEnd )
        //      {
        //          tree_detach(bl);

        //          //
        //          block_header* frontFence = (block_header*)alignedEnd;
        //          frontFence->prev(0);
        //          frontFence->size(0);
        //          frontFence->set_used();
        //          block_header* frontFreeBlock = (block_header*)frontFence->mem();
        //          frontFreeBlock->prev(frontFence);
        //          frontFreeBlock->set_unused();
        //          frontFreeBlock->next(bl->next()); // set's the size
        //          bl->next()->prev(frontFreeBlock);
        //          tree_attach(frontFreeBlock);

        //          //
        //          block_header* backFence = (block_header*)(alignedStart-sizeof(block_header));
        //          block_header* backFreeBlock = bl;
        //          backFreeBlock->next(backFence); // set's the size

        //          backFence->prev(backFreeBlock);
        //          backFence->size(0);
        //          backFence->set_used();
        //          tree_attach(backFreeBlock);

        //          memset(alignedStart,0xff,sizeof(block_header));
        //          tree_system_free(alignedStart, alignedEnd - alignedStart);
        //      }
        //  }
        //}
    }

    size_t HphaAllocatorPimpl::tree_ptr_size(void* ptr) const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        block_header* bl = ptr_get_block_header(ptr);
        if (bl->used()) // add a magic number to avoid better bad pointer data.
        {
            return bl->size();
        }
        else
        {
            return 0;
        }
    }

    size_t HphaAllocatorPimpl::tree_get_max_allocation() const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        return mFreeTree.maximum()->get_block()->size();
    }

    size_t HphaAllocatorPimpl::tree_get_unused_memory(bool isPrint) const
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        size_t unusedMemory = 0;
        for (free_node_tree::const_iterator it = mFreeTree.begin(); it != mFreeTree.end(); ++it)
        {
            unusedMemory += it->get_block()->size();
            if (isPrint)
            {
                AZ_TracePrintf("System", "Unused Treenode %p size: %zu\n", it->get_block()->mem(), it->get_block()->size());
            }
        }
        return unusedMemory;
    }

    void HphaAllocatorPimpl::tree_purge()
    {
#ifdef MULTITHREADED
        AZStd::lock_guard<AZStd::recursive_mutex> lock(mTreeMutex);
#endif
        free_node_tree::iterator node = mFreeTree.begin();
        free_node_tree::iterator end = mFreeTree.end();
        while (node != end)
        {
            block_header* cur = node->get_block();
            ++node;
            tree_purge_block(cur);
        }
    }

    HphaAllocatorPimpl::pointer HphaAllocatorPimpl::allocate(size_type byteSize, align_type alignment)
    {
        pointer address = alloc(byteSize, static_cast<size_t>(alignment));
        if (address == nullptr)
        {
            purge();
            address = alloc(byteSize, static_cast<size_t>(alignment));
        }
#if defined(AZ_ENABLE_TRACING)
        if (address)
        {
            // Cannot store the actual requested size because that size is no longer knowable. This
            // allocator will size-up certain allocations and loose track of the original size.
            // In order to keep this counter sane, we will not be tracking this small size-up, which
            // will under-report fragmentation. If needed, fragmentation can be tracked separately
            // through the allocation record.
            const size_t sizedUpSize = get_allocated_size(address);
            AddAllocationRecord(address, sizedUpSize, sizedUpSize, alignment, 1);
        }
#endif
        return address;
    }

    void HphaAllocatorPimpl::deallocate(pointer ptr, size_type byteSize, align_type alignment)
    {
        if (ptr == nullptr)
        {
            return;
        }
#if defined(AZ_ENABLE_TRACING)
        const size_type originalAllocation = get_allocated_size(ptr);
        RemoveAllocationRecord(ptr, originalAllocation, originalAllocation);
#endif
        if (byteSize == 0)
        {
            free(ptr);
        }
        else if (alignment == 0)
        {
            free(ptr, byteSize);
        }
        else
        {
            free(ptr, byteSize, alignment);
        }
    }

    HphaAllocatorPimpl::pointer HphaAllocatorPimpl::reallocate(pointer ptr, size_type newSize, align_type alignment)
    {
#if defined(AZ_ENABLE_TRACING)
        const size_type originalAllocation = get_allocated_size(ptr);
        RemoveAllocationRecord(ptr, originalAllocation, originalAllocation);
#endif
        pointer address = realloc(ptr, newSize, static_cast<size_t>(alignment));
        if (address == nullptr && newSize > 0)
        {
            purge();
            address = realloc(ptr, newSize, static_cast<size_t>(alignment));
        }
#if defined(AZ_ENABLE_TRACING)
        if (address)
        {
            const size_t sizedUpSize = get_allocated_size(address);
            AddAllocationRecord(address, sizedUpSize, sizedUpSize, alignment, 1);
        }
#endif
        return address;
    }

    HphaAllocatorPimpl::size_type HphaAllocatorPimpl::get_allocated_size(pointer ptr, [[maybe_unused]] align_type alignment) const
    {
        return size(ptr);
    }

    void HphaAllocatorPimpl::Merge([[maybe_unused]] IAllocator* aOther)
    {
        HphaAllocatorPimpl* other = azrtti_cast<HphaAllocatorPimpl*>(aOther);
        HPPA_ASSERT(other);
        // Transfer the buckets
        // Transfer tree

        // Transfer the tracking records
        RecordingsMove(other);
    }

    void HphaAllocatorPimpl::GarbageCollect()
    {
        purge();
    }

    IAllocatorWithTracking* CreateHphaAllocatorPimpl(IAllocator& subAllocator)
    {
        // We use the AZStd::stateless_allocator for the allocation of this object to prevent it from showing up as a leak
        // in other allocators.
        HphaAllocatorPimpl* allocatorMemory = reinterpret_cast<HphaAllocatorPimpl*>(
            AZStd::stateless_allocator().allocate(sizeof(HphaAllocatorPimpl), alignof(HphaAllocatorPimpl)));
        return new(allocatorMemory)HphaAllocatorPimpl(subAllocator);
    }

    void DestroyHphaAllocatorPimpl([[maybe_unused]] IAllocator& subAllocator, IAllocatorWithTracking* allocator)
    {
        HphaAllocatorPimpl* allocatorImpl = azrtti_cast<HphaAllocatorPimpl*>(allocator);
        allocatorImpl->~HphaAllocatorPimpl();
        AZStd::stateless_allocator().deallocate(allocatorImpl, sizeof(HphaAllocatorPimpl));
    }

} // namespace AZ
