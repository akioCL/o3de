/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/functional.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AZ
{
    class IDataSerializer;
    using IDataSerializerDeleter = AZStd::function<void(IDataSerializer* ptr)>;
    using IDataSerializerPtr = AZStd::unique_ptr<IDataSerializer, IDataSerializerDeleter>;

    namespace IO
    {
        class GenericStream;
    }

    /**
     * Interface for data serialization. Should be implemented for lowest level
     * of data. Once this implementation is detected, the class will not be drilled
     * down. We will assume this implementation covers the full class.
     */
    class IDataSerializer
    {
    public:
        static IDataSerializerDeleter CreateDefaultDeleteDeleter();
        static IDataSerializerDeleter CreateNoDeleteDeleter();

        virtual ~IDataSerializer() {}

        /// Store the class data into a stream.
        virtual size_t  Save(const void* classPtr, IO::GenericStream& stream, bool isDataBigEndian = false) = 0;

        /// Load the class data from a stream.
        virtual bool    Load(void* classPtr, IO::GenericStream& stream, unsigned int version, bool isDataBigEndian = false) = 0;

        /// Convert binary data to text.
        virtual size_t  DataToText(IO::GenericStream& in, IO::GenericStream& out, bool isDataBigEndian /*= false*/) = 0;

        /// Convert text data to binary, to support loading old version formats. We must respect text version if the text->binary format has changed!
        virtual size_t  TextToData(const char* text, unsigned int textVersion, IO::GenericStream& stream, bool isDataBigEndian = false) = 0;

        /// Compares two instances of the type.
        /// \return true if they match.
        /// Note: Input pointers are assumed to point to valid instances of the class.
        virtual bool    CompareValueData(const void* lhs, const void* rhs) = 0;

        /// Optional post processing of the cloned data to deal with members that are not serialize-reflected.
        virtual void PostClone(void* /*classPtr*/) {}
    };

}   // namespace AZ
