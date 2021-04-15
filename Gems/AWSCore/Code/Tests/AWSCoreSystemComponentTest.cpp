/*
 * All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
 * its licensors.
 *
 * For complete copyright and license terms please see the LICENSE at the root of this
 * distribution (the "License"). All use of this software is governed by the License,
 * or, if provided, by the license below or the license accompanying this file. Do not
 * remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 */

#include <AzCore/Jobs/JobManager.h>
#include <AzCore/Jobs/JobContext.h>
#include <AzCore/Jobs/JobManagerBus.h>
#include <AzCore/Jobs/JobCancelGroup.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzTest/AzTest.h>

#include <AWSCoreSystemComponent.h>
#include <Configuration/AWSCoreConfiguration.h>
#include <Credential/AWSCredentialManager.h>
#include <Framework/AWSApiJob.h>
#include <ResourceMapping/AWSResourceMappingManager.h>
#include <TestFramework/AWSCoreFixture.h>

//#include <aws/gamelift/server/GameLiftServerAPI.h>

using namespace AWSCore;

class AWSCoreNotificationsBusMock
    : protected AWSCoreNotificationsBus::Handler
{
public:
    AZ_CLASS_ALLOCATOR(AWSCoreNotificationsBusMock, AZ::SystemAllocator, 0);

    AWSCoreNotificationsBusMock()
        : m_sdkInitialized(0)
        , m_sdkShutdownStarted(0)
    {
        AWSCoreNotificationsBus::Handler::BusConnect();
    }

    ~AWSCoreNotificationsBusMock()
    {
        AWSCoreNotificationsBus::Handler::BusDisconnect();
    }

    //////////////////////////////////////////////////////////////////////////
    // handles AWSCoreNotificationsBus
    void OnSDKInitialized() override { m_sdkInitialized++; }
    void OnSDKShutdownStarted() override {  m_sdkShutdownStarted++; }

    int m_sdkInitialized;
    int m_sdkShutdownStarted;
};

class AWSCoreSystemComponentTest
    : public AWSCoreFixture
{
protected:
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
    AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;

    void SetUp() override
    {
        AWSCoreFixture::SetUp();

        m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
        m_serializeContext->CreateEditContext();
        m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
        m_componentDescriptor.reset(AWSCoreSystemComponent::CreateDescriptor());
        m_componentDescriptor->Reflect(m_serializeContext.get());
        m_componentDescriptor->Reflect(m_behaviorContext.get());

        m_entity = aznew AZ::Entity();
        m_coreSystemsComponent.reset(m_entity->CreateComponent<AWSCoreSystemComponent>());
    }

    void TearDown() override
    {
        m_entity->RemoveComponent(m_coreSystemsComponent.get());
        m_coreSystemsComponent.reset();
        delete m_entity;
        m_entity = nullptr;

        m_coreSystemsComponent.reset();
        m_componentDescriptor.reset();
        m_behaviorContext.reset();
        m_serializeContext.reset();

        AWSCoreFixture::TearDown();
    }

public:
    AZStd::unique_ptr<AWSCoreSystemComponent> m_coreSystemsComponent;
    AZ::Entity* m_entity;
    AWSCoreNotificationsBusMock m_notifications;
};

TEST_F(AWSCoreSystemComponentTest, ComponentActivateTest)
{
    //auto initOutcome = Aws::GameLift::Server::InitSDK();
    AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(2000));
}
