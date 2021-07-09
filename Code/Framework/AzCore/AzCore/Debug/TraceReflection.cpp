/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/TraceReflection.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Component/TickBus.h>

#define AZ_EBUS_BEHAVIOR_BINDER_NO_CONECTIONS(_Handler,_Uuid,_Allocator,...)\
    AZ_CLASS_ALLOCATOR(_Handler,_Allocator,0)\
    AZ_RTTI(_Handler,_Uuid,AZ::BehaviorEBusHandler)\
    typedef _Handler ThisType;\
    using EventFunctionsParameterPack = AZStd::Internal::pack_traits_arg_sequence<AZ_BEHAVIOR_EBUS_1ARG_UNPACKER(AZ_BEHAVIOR_EBUS_EVENT_FUNCTION_TYPE, __VA_ARGS__)>;\
    enum {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_ENUM, AZ_EBUS_SEQ(__VA_ARGS__))\
        FN_MAX\
    };\
    int GetFunctionIndex(const char* functionName) const override {\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_FUNC_INDEX, AZ_EBUS_SEQ(__VA_ARGS__))\
        return -1;\
    }\
    _Handler(){\
        m_events.resize(FN_MAX);\
        AZ_SEQ_FOR_EACH(AZ_BEHAVIOR_EBUS_REG_EVENT, AZ_EBUS_SEQ(__VA_ARGS__))\
    }

namespace AZ
{
    namespace Debug
    {
        class TraceMessageBusHandler
            : public AZ::Debug::TraceMessageBus::Handler
            , public AZ::BehaviorEBusHandler
            , private AZ::TickBus::Handler
        {
        public:
            AZ_EBUS_BEHAVIOR_BINDER_NO_CONECTIONS(TraceMessageBusHandler, "{5CDBAF09-5EB0-48AC-B327-2AF8601BB550}", AZ::SystemAllocator
                , OnPreAssert
                , OnPreError
                , OnPreWarning
                , OnAssert
                , OnError
                , OnWarning
                , OnException
                , OnPrintf
                , OnOutput
            );

            void Disconnect() override {
                AZ::Debug::TraceMessageBus::Handler::BusDisconnect();
                AZ::TickBus::Handler::BusDisconnect();
            }
            bool Connect(AZ::BehaviorValueParameter* id = nullptr) override {
                AZ::TickBus::Handler::BusConnect();
                return AZ::Internal::EBusConnector<AZ::Debug::TraceMessageBus::Handler>::Connect(this, id);
            }
            bool IsConnected() override {
                return AZ::Internal::EBusConnector<AZ::Debug::TraceMessageBus::Handler>::IsConnected(this);
            }
            bool IsConnectedId(AZ::BehaviorValueParameter* id) override {
                return AZ::Internal::EBusConnector<AZ::Debug::TraceMessageBus::Handler>::IsConnectedId(this, id);
            }

            // TraceMessageBus
            /*
             *  Note: Since at editor runtime there is already have a handler, for automation (OnPreAssert, OnPreWarning, OnPreWarning)
             *  must be used instead of (OnAssert, OnWarning, OnError)
            */
            bool OnPreAssert(const char* fileName, int line, const char* func, const char* message) override;
            bool OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message) override;
            bool OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message) override;
            bool OnAssert(const char* message) override;
            bool OnError(const char* window, const char* message) override;
            bool OnWarning(const char* window, const char* message) override;
            bool OnException(const char* message) override;
            bool OnPrintf(const char* window, const char* message) override;
            bool OnOutput(const char* window, const char* message) override;

        private:
            template<class R, class... Args>
            R CallResultReturn(const R& defaultReturnValue, int index, Args&&... args) const
            {
                R returnVal = defaultReturnValue;
                CallResult(returnVal, index, AZStd::forward<Args>(args)...);
                return returnVal;
            }

            // AZ::TickBus::Handler overrides ...
            void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
            int GetTickOrder() override;

            bool QueueMessageCall(AZStd::function<void()> messageCall);
            void FlushMessageCalls();

            AZStd::list<AZStd::function<void()>> m_messageCalls;
            AZStd::mutex m_lock;
        };

        //////////////////////////////////////////////////////////////////////////
        // TraceMessageBusHandler Implementation
        inline bool TraceMessageBusHandler::OnPreAssert(const char* fileName, int line, const char* func, const char* message)
        {
            return QueueMessageCall(
                [this, fileNameString = AZStd::string(fileName), line, funcString = AZStd::string(func), messageString = AZStd::string(message)]()
                {
                    CallResultReturn(false, FN_OnPreAssert, fileNameString, line, funcString, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnPreError(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            return QueueMessageCall(
                [this, windowString = AZStd::string(window), fileNameString = AZStd::string(fileName), line, funcString = AZStd::string(func), messageString = AZStd::string(message)]()
                {
                    CallResultReturn(false, FN_OnPreError, windowString, fileNameString, line, funcString, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnPreWarning(const char* window, const char* fileName, int line, const char* func, const char* message)
        {
            return QueueMessageCall(
                [this, windowString = AZStd::string(window), fileNameString = AZStd::string(fileName), line, funcString = AZStd::string(func), messageString = AZStd::string(message)]()
                {
                    return CallResultReturn(false, FN_OnPreWarning, windowString, fileNameString, line, funcString, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnAssert(const char* message)
        {
            return QueueMessageCall(
                [this, messageString = AZStd::string(message)]()
                {
                    return CallResultReturn(false, FN_OnAssert, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnError(const char* window, const char* message)
        {
            return QueueMessageCall(
                [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
                {
                    return CallResultReturn(false, FN_OnError, windowString, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnWarning(const char* window, const char* message)
        {
            return QueueMessageCall(
                [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
                {
                    return CallResultReturn(false, FN_OnWarning, windowString, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnException(const char* message)
        {
            return QueueMessageCall(
                [this, messageString = AZStd::string(message)]()
                {
                    return CallResultReturn(false, FN_OnException, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnPrintf(const char* window, const char* message)
        {
            return QueueMessageCall(
                [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
                {
                    return CallResultReturn(false, FN_OnPrintf, windowString, messageString);
                });
        }

        inline bool TraceMessageBusHandler::OnOutput(const char* window, const char* message)
        {
            return QueueMessageCall(
                [this, windowString = AZStd::string(window), messageString = AZStd::string(message)]()
                {
                    return CallResultReturn(false, FN_OnOutput, windowString, messageString);
                });
        }

        void TraceMessageBusHandler::OnTick(
            [[maybe_unused]] float deltaTime,
            [[maybe_unused]] AZ::ScriptTimePoint time)
        {
            FlushMessageCalls();
        }

        int TraceMessageBusHandler::GetTickOrder()
        {
            return AZ::TICK_LAST;
        }

        bool TraceMessageBusHandler::QueueMessageCall(AZStd::function<void()> messageCall)
        {
            AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
            m_messageCalls.push_back(messageCall);
            return true;
        }

        void TraceMessageBusHandler::FlushMessageCalls()
        {
            AZStd::list<AZStd::function<void()>> messageCalls;
            {
                AZStd::lock_guard<decltype(m_lock)> lock(m_lock);
                if (m_messageCalls.empty())
                {
                    return;
                }
                messageCalls = AZStd::move(m_messageCalls); // Move calls to release the lock as soon as possible
                m_messageCalls = AZStd::list<AZStd::function<void()>>();
            }

            for (auto& messageCall : messageCalls)
            {
                messageCall();
            }
        }

        void TraceReflect(ReflectContext* context)
        {
            if (BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->EBus<AZ::Debug::TraceMessageBus>("TraceMessageBus")
                    ->Attribute(AZ::Script::Attributes::Module, "debug")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Handler<TraceMessageBusHandler>()
                    ;
            }
        }
    }
}
