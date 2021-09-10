/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <IMultiplayerConnectionMock.h>
#include <MockInterfaces.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UnitTest/UnitTest.h>
#include <AzNetworking/Serialization/NetworkInputSerializer.h>
#include <AzNetworking/Serialization/NetworkOutputSerializer.h>
#include <AzTest/AzTest.h>
#include <Multiplayer/IMultiplayer.h>
#include <Multiplayer/Components/NetBindComponent.h>
#include <Multiplayer/Components/NetworkHierarchyChildComponent.h>
#include <Multiplayer/Components/NetworkHierarchyRootComponent.h>
#include <Multiplayer/Components/NetworkTransformComponent.h>
#include <NetworkEntity/NetworkEntityAuthorityTracker.h>
#include <NetworkEntity/NetworkEntityTracker.h>
#include <NetworkEntity/EntityReplication/EntityReplicationManager.h>
#include <NetworkEntity/EntityReplication/EntityReplicator.h>

#pragma optimize("", off)

namespace Multiplayer
{
    using namespace testing;
    using namespace ::UnitTest;

    class HierarchyTests
        : public AllocatorsFixture
    {
    public:
        void SetUp() override
        {
            SetupAllocator();
            AZ::NameDictionary::Create();

            m_mockComponentApplicationRequests = AZStd::make_unique<MockComponentApplicationRequests>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_mockComponentApplicationRequests.get());

            ON_CALL(*m_mockComponentApplicationRequests, AddEntity(_)).WillByDefault(Return(true));

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_netBindDescriptor.reset(NetBindComponent::CreateDescriptor());
            m_netBindDescriptor->Reflect(m_serializeContext.get());

            m_hierarchyRootDescriptor.reset(NetworkHierarchyRootComponent::CreateDescriptor());
            m_hierarchyRootDescriptor->Reflect(m_serializeContext.get());

            m_hierarchyChildDescriptor.reset(NetworkHierarchyChildComponent::CreateDescriptor());
            m_hierarchyChildDescriptor->Reflect(m_serializeContext.get());

            m_netTransformDescriptor.reset(NetworkTransformComponent::CreateDescriptor());
            m_netTransformDescriptor->Reflect(m_serializeContext.get());

            m_mockMultiplayer = AZStd::make_unique<MockMultiplayer>();
            AZ::Interface<IMultiplayer>::Register(m_mockMultiplayer.get());

            // Create space for replication stats
            // Without Multiplayer::RegisterMultiplayerComponents() the stats go to invalid id, which is fine for unit tests
            GetMultiplayer()->GetStats().ReserveComponentStats(Multiplayer::InvalidNetComponentId, 50, 0);

            m_mockNetworkEntityManager = AZStd::make_unique<MockNetworkEntityManager>();

            m_mockTime = AZStd::make_unique<MockTime>();
            AZ::Interface<AZ::ITime>::Register(m_mockTime.get());

            m_mockNetworkTime = AZStd::make_unique<MockNetworkTime>();
            AZ::Interface<INetworkTime>::Register(m_mockNetworkTime.get());

            ON_CALL(*m_mockMultiplayer, GetNetworkEntityManager()).WillByDefault(Return(m_mockNetworkEntityManager.get()));

            const IpAddress address("localhost", 1, ProtocolType::Udp);
            m_mockConnection = AZStd::make_unique<IMultiplayerConnectionMock>(ConnectionId{ 1 }, address, ConnectionRole::Connector);
            m_mockConnectionListener = AZStd::make_unique<MockConnectionListener>();

            m_networkEntityTracker = AZStd::make_unique<NetworkEntityTracker>();
            m_networkEntityAuthorityTracker = AZStd::make_unique<NetworkEntityAuthorityTracker>(*m_mockNetworkEntityManager);
            ON_CALL(*m_mockNetworkEntityManager, GetNetworkEntityAuthorityTracker()).WillByDefault(Return(m_networkEntityAuthorityTracker.get()));

            m_entityReplicationManager = AZStd::make_unique<EntityReplicationManager>(*m_mockConnection, *m_mockConnectionListener, EntityReplicationManager::Mode::LocalClientToRemoteServer);
        }

        void TearDown() override
        {
            m_entityReplicationManager.reset();

            m_mockConnection.reset();
            m_mockConnectionListener.reset();
            m_networkEntityTracker.reset();
            m_networkEntityAuthorityTracker.reset();

            AZ::Interface<INetworkTime>::Unregister(m_mockNetworkTime.get());
            AZ::Interface<AZ::ITime>::Unregister(m_mockTime.get());
            AZ::Interface<IMultiplayer>::Unregister(m_mockMultiplayer.get());
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(m_mockComponentApplicationRequests.get());

            m_mockTime.reset();

            m_mockNetworkEntityManager.reset();
            m_mockMultiplayer.reset();

            m_netTransformDescriptor.reset();
            m_hierarchyRootDescriptor.reset();
            m_hierarchyChildDescriptor.reset();
            m_netBindDescriptor.reset();
            m_serializeContext.reset();
            m_mockComponentApplicationRequests.reset();

            AZ::NameDictionary::Destroy();
            TeardownAllocator();
        }

        AZStd::unique_ptr<MockComponentApplicationRequests> m_mockComponentApplicationRequests;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyRootDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyChildDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netTransformDescriptor;

        AZStd::unique_ptr<MockMultiplayer> m_mockMultiplayer;
        AZStd::unique_ptr<MockNetworkEntityManager> m_mockNetworkEntityManager;
        AZStd::unique_ptr<MockTime> m_mockTime;
        AZStd::unique_ptr<MockNetworkTime> m_mockNetworkTime;

        AZStd::unique_ptr<IMultiplayerConnectionMock> m_mockConnection;
        AZStd::unique_ptr<MockConnectionListener> m_mockConnectionListener;
        AZStd::unique_ptr<NetworkEntityTracker> m_networkEntityTracker;
        AZStd::unique_ptr<NetworkEntityAuthorityTracker> m_networkEntityAuthorityTracker;

        AZStd::unique_ptr<EntityReplicationManager> m_entityReplicationManager;

        void SetupEntity(AZ::Entity& entity, NetEntityId netId)
        {
            const auto netBindComponent = entity.FindComponent<Multiplayer::NetBindComponent>();
            EXPECT_NE(netBindComponent, nullptr);
            netBindComponent->PreInit(&entity, PrefabEntityId{ AZ::Name("test"), 1 }, netId, NetEntityRole::Client);
        }

        void StopEntity(const AZ::Entity& entity)
        {
            const auto netBindComponent = entity.FindComponent<Multiplayer::NetBindComponent>();
            EXPECT_NE(netBindComponent, nullptr);
            netBindComponent->StopEntity();
        }

        void CreateEntityWithRootHierarchy(AZ::Entity& rootEntity)
        {
            rootEntity.CreateComponent<NetBindComponent>();
            rootEntity.CreateComponent<NetworkTransformComponent>();
            rootEntity.CreateComponent<NetworkHierarchyRootComponent>();
            rootEntity.Init();
        }

        void CreateEntityWithChildHierarchy(AZ::Entity& childEntity)
        {
            childEntity.CreateComponent<NetBindComponent>();
            childEntity.CreateComponent<NetworkTransformComponent>();
            childEntity.CreateComponent<NetworkHierarchyChildComponent>();
            childEntity.Init();
        }

        void SetParentIdOnNetworkTransform(AZ::Entity& entity, uint32_t netParentId)
        {
            /* Derived from NetworkTransformComponent.AutoComponent.xml */
            constexpr int totalBits = 6 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::Count*/;
            constexpr int parentIdBit = 4 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::parentEntityId_DirtyFlag*/;

            ReplicationRecord currentRecord;
            currentRecord.m_authorityToClient.AddBits(totalBits);
            currentRecord.m_authorityToClient.SetBit(parentIdBit, true);

            AZStd::array<uint8_t, 100> buffer = {};
            NetworkInputSerializer inSerializer(buffer.begin(), aznumeric_cast<uint32_t>(buffer.size()));
            inSerializer.Serialize(netParentId, "parentEntityId", /* Derived from NetworkTransformComponent.AutoComponent.xml */
                AZStd::numeric_limits<uint32_t>::min(), AZStd::numeric_limits<uint32_t>::max());

            NetworkOutputSerializer outSerializer(buffer.begin(), aznumeric_cast<uint32_t>(buffer.size()));

            entity.FindComponent<NetworkTransformComponent>()->SerializeStateDeltaMessage(currentRecord, outSerializer);
            // now the parent id is in the component
        }
    };

    /*
     * Test NetBindComponent activation. This must work before more complicated tests.
     */
    TEST_F(HierarchyTests, NetBindComponent_Activate)
    {
        AZ::Entity entity;
        entity.CreateComponent<NetBindComponent>();

        entity.Init();
        SetupEntity(entity, NetEntityId{ 1 });
        entity.Activate();

        StopEntity(entity);

        entity.Deactivate();
    }

    /*
     * Hierarchy test - a child entity on a client delaying activation until its hierarchical parent has been activated
     */
    TEST_F(HierarchyTests, EntityReplicator_DontActivate_BeforeParent)
    {
        // Create a child entity that will be tested for activation inside a hierarchy
        AZ::Entity childEntity;
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 });
        // child entity is not activated on purpose here, we are about to test conditional activation check

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, 1);

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(&childEntity, m_networkEntityTracker.get());
        EntityReplicator entityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
        entityReplicator.Initialize(childHandle);

        // Entity replicator should not be ready to activate the entity because its parent does not exist
        EXPECT_EQ(entityReplicator.IsReadyToActivate(), false);
    }

    /*
     * Hierarchy test - a child entity on a client allowing activation when its hierarchical parent is active
     */
    TEST_F(HierarchyTests, EntityReplicator_Activate_AfterParent)
    {
        AZ::Entity childEntity;
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 });

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, 1);

        // Create an entity replicator for the child entity
        const NetworkEntityHandle childHandle(&childEntity, m_networkEntityTracker.get());
        EntityReplicator entityReplicator(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
        entityReplicator.Initialize(childHandle);

        // Now let's create a parent entity and activate it
        AZ::Entity parentEntity;
        CreateEntityWithRootHierarchy(parentEntity);
        SetupEntity(parentEntity, NetEntityId{ 1 });
        parentEntity.Activate();

        ConstNetworkEntityHandle parentHandle(&parentEntity, m_networkEntityTracker.get());
        ON_CALL(*m_mockNetworkEntityManager, GetEntity(_)).WillByDefault(Return(parentHandle));

        // The child should be ready to be activated
        EXPECT_EQ(entityReplicator.IsReadyToActivate(), true);

        StopEntity(parentEntity);

        parentEntity.Deactivate();
    }
}

