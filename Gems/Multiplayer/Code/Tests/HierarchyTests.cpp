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
#include <AzFramework/Components/TransformComponent.h>
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

            m_mockComponentApplicationRequests = AZStd::make_unique<NiceMock<MockComponentApplicationRequests>>();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(m_mockComponentApplicationRequests.get());

            ON_CALL(*m_mockComponentApplicationRequests, AddEntity(_)).WillByDefault(Invoke(this, &HierarchyTests::AddEntity));
            ON_CALL(*m_mockComponentApplicationRequests, FindEntity(_)).WillByDefault(Invoke(this, &HierarchyTests::FindEntity));

            // register components involved in testing
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();

            m_transformDescriptor.reset(AzFramework::TransformComponent::CreateDescriptor());
            m_transformDescriptor->Reflect(m_serializeContext.get());

            m_netBindDescriptor.reset(NetBindComponent::CreateDescriptor());
            m_netBindDescriptor->Reflect(m_serializeContext.get());

            m_hierarchyRootDescriptor.reset(NetworkHierarchyRootComponent::CreateDescriptor());
            m_hierarchyRootDescriptor->Reflect(m_serializeContext.get());

            m_hierarchyChildDescriptor.reset(NetworkHierarchyChildComponent::CreateDescriptor());
            m_hierarchyChildDescriptor->Reflect(m_serializeContext.get());

            m_netTransformDescriptor.reset(NetworkTransformComponent::CreateDescriptor());
            m_netTransformDescriptor->Reflect(m_serializeContext.get());

            m_mockMultiplayer = AZStd::make_unique<NiceMock<MockMultiplayer>>();
            AZ::Interface<IMultiplayer>::Register(m_mockMultiplayer.get());

            // Create space for replication stats
            // Without Multiplayer::RegisterMultiplayerComponents() the stats go to invalid id, which is fine for unit tests
            GetMultiplayer()->GetStats().ReserveComponentStats(Multiplayer::InvalidNetComponentId, 50, 0);

            m_mockNetworkEntityManager = AZStd::make_unique<NiceMock<MockNetworkEntityManager>>();

            m_mockTime = AZStd::make_unique<MockTime>();
            AZ::Interface<AZ::ITime>::Register(m_mockTime.get());

            m_mockNetworkTime = AZStd::make_unique<NiceMock<MockNetworkTime>>();
            AZ::Interface<INetworkTime>::Register(m_mockNetworkTime.get());

            ON_CALL(*m_mockMultiplayer, GetNetworkEntityManager()).WillByDefault(Return(m_mockNetworkEntityManager.get()));

            const IpAddress address("localhost", 1, ProtocolType::Udp);
            m_mockConnection = AZStd::make_unique<NiceMock<IMultiplayerConnectionMock>>(ConnectionId{ 1 }, address, ConnectionRole::Connector);
            m_mockConnectionListener = AZStd::make_unique<MockConnectionListener>();

            m_networkEntityTracker = AZStd::make_unique<NetworkEntityTracker>();
            m_networkEntityAuthorityTracker = AZStd::make_unique<NetworkEntityAuthorityTracker>(*m_mockNetworkEntityManager);
            ON_CALL(*m_mockNetworkEntityManager, GetNetworkEntityAuthorityTracker()).WillByDefault(Return(m_networkEntityAuthorityTracker.get()));

            m_entityReplicationManager = AZStd::make_unique<EntityReplicationManager>(*m_mockConnection, *m_mockConnectionListener, EntityReplicationManager::Mode::LocalClientToRemoteServer);
        }

        void TearDown() override
        {
            m_entities.clear();

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

            m_transformDescriptor.reset();
            m_netTransformDescriptor.reset();
            m_hierarchyRootDescriptor.reset();
            m_hierarchyChildDescriptor.reset();
            m_netBindDescriptor.reset();
            m_serializeContext.reset();
            m_mockComponentApplicationRequests.reset();

            AZ::NameDictionary::Destroy();
            TeardownAllocator();
        }

        AZStd::unique_ptr<NiceMock<MockComponentApplicationRequests>> m_mockComponentApplicationRequests;
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_transformDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netBindDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyRootDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_hierarchyChildDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_netTransformDescriptor;

        AZStd::unique_ptr<NiceMock<MockMultiplayer>> m_mockMultiplayer;
        AZStd::unique_ptr<NiceMock<MockNetworkEntityManager>> m_mockNetworkEntityManager;
        AZStd::unique_ptr<MockTime> m_mockTime;
        AZStd::unique_ptr<NiceMock<MockNetworkTime>> m_mockNetworkTime;

        AZStd::unique_ptr<NiceMock<IMultiplayerConnectionMock>> m_mockConnection;
        AZStd::unique_ptr<MockConnectionListener> m_mockConnectionListener;
        AZStd::unique_ptr<NetworkEntityTracker> m_networkEntityTracker;
        AZStd::unique_ptr<NetworkEntityAuthorityTracker> m_networkEntityAuthorityTracker;

        AZStd::unique_ptr<EntityReplicationManager> m_entityReplicationManager;

        AZStd::map<AZ::EntityId, AZ::Entity*> m_entities;

        bool AddEntity(AZ::Entity* entity)
        {
            m_entities[entity->GetId()] = entity;
            return true;
        }

        AZ::Entity* FindEntity(AZ::EntityId entityId)
        {
            const auto iterator = m_entities.find(entityId);
            if (iterator != m_entities.end())
            {
                return iterator->second;
            }

            return nullptr;
        }

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
            rootEntity.CreateComponent<AzFramework::TransformComponent>();
            rootEntity.CreateComponent<NetBindComponent>();
            rootEntity.CreateComponent<NetworkTransformComponent>();
            rootEntity.CreateComponent<NetworkHierarchyRootComponent>();
            rootEntity.Init();
        }

        void CreateEntityWithChildHierarchy(AZ::Entity& childEntity)
        {
            childEntity.CreateComponent<AzFramework::TransformComponent>();
            childEntity.CreateComponent<NetBindComponent>();
            childEntity.CreateComponent<NetworkTransformComponent>();
            childEntity.CreateComponent<NetworkHierarchyChildComponent>();
            childEntity.Init();
        }

        void SetParentIdOnNetworkTransform(const AZ::Entity& entity, NetEntityId netParentId)
        {
            /* Derived from NetworkTransformComponent.AutoComponent.xml */
            constexpr int totalBits = 6 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::Count*/;
            constexpr int parentIdBit = 4 /*NetworkTransformComponentInternal::AuthorityToClientDirtyEnum::parentEntityId_DirtyFlag*/;

            ReplicationRecord currentRecord;
            currentRecord.m_authorityToClient.AddBits(totalBits);
            currentRecord.m_authorityToClient.SetBit(parentIdBit, true);

            constexpr uint32_t bufferSize = 100;
            AZStd::array<uint8_t, bufferSize> buffer = {};
            NetworkInputSerializer inSerializer(buffer.begin(), bufferSize);
            inSerializer.Serialize(reinterpret_cast<uint32_t&>(netParentId),
                "parentEntityId", /* Derived from NetworkTransformComponent.AutoComponent.xml */
                AZStd::numeric_limits<uint32_t>::min(), AZStd::numeric_limits<uint32_t>::max());

            NetworkOutputSerializer outSerializer(buffer.begin(), bufferSize);

            entity.FindComponent<NetworkTransformComponent>()->SerializeStateDeltaMessage(currentRecord, outSerializer);
            // now the parent id is in the component
        }

        struct EntityInfo
        {
            enum class Role
            {
                Root,
                Child,
                None
            };

            EntityInfo(AZ::Entity& entity, NetEntityId netId, Role role)
                : m_entity(entity)
                , m_netId(netId)
                , m_role(role)
            {
            }

            AZ::Entity& m_entity;
            NetEntityId m_netId;
            AZStd::unique_ptr<EntityReplicator> m_replicator;
            Role m_role = Role::None;
        };

        void PopulateHierarchicalEntity(const EntityInfo& entityInfo)
        {
            entityInfo.m_entity.CreateComponent<AzFramework::TransformComponent>();
            entityInfo.m_entity.CreateComponent<NetBindComponent>();
            entityInfo.m_entity.CreateComponent<NetworkTransformComponent>();
            switch (entityInfo.m_role)
            {
            case EntityInfo::Role::Root:
                entityInfo.m_entity.CreateComponent<NetworkHierarchyRootComponent>();
                break;
            case EntityInfo::Role::Child:
                entityInfo.m_entity.CreateComponent<NetworkHierarchyChildComponent>();
                break;
            case EntityInfo::Role::None:
                break;
            }
            entityInfo.m_entity.Init();
        }

        void CreateSimpleHierarchy(EntityInfo& root, EntityInfo& child)
        {
            PopulateHierarchicalEntity(root);
            PopulateHierarchicalEntity(child);

            SetupEntity(root.m_entity, root.m_netId);
            SetupEntity(child.m_entity, child.m_netId);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(child.m_entity, root.m_netId);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(&child.m_entity, m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(&root.m_entity, m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity.Activate();
            child.m_entity.Activate();
        }

        void CreateDeepHierarchy(EntityInfo& root, EntityInfo& child, EntityInfo& childOfChild)
        {
            PopulateHierarchicalEntity(root);
            PopulateHierarchicalEntity(child);
            PopulateHierarchicalEntity(childOfChild);

            SetupEntity(root.m_entity, root.m_netId);
            SetupEntity(child.m_entity, child.m_netId);
            SetupEntity(childOfChild.m_entity, childOfChild.m_netId);

            // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
            SetParentIdOnNetworkTransform(child.m_entity, root.m_netId);
            SetParentIdOnNetworkTransform(childOfChild.m_entity, child.m_netId);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childOfChildHandle(&childOfChild.m_entity, m_networkEntityTracker.get());
            childOfChild.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childOfChildHandle);
            childOfChild.m_replicator->Initialize(childOfChildHandle);

            // Create an entity replicator for the child entity
            const NetworkEntityHandle childHandle(&child.m_entity, m_networkEntityTracker.get());
            child.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, childHandle);
            child.m_replicator->Initialize(childHandle);

            // Create an entity replicator for the root entity
            const NetworkEntityHandle rootHandle(&root.m_entity, m_networkEntityTracker.get());
            root.m_replicator = AZStd::make_unique<EntityReplicator>(*m_entityReplicationManager, m_mockConnection.get(), NetEntityRole::Authority, rootHandle);
            root.m_replicator->Initialize(rootHandle);

            root.m_entity.Activate();
            child.m_entity.Activate();
            childOfChild.m_entity.Activate();
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
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });

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
    TEST_F(HierarchyTests, EntityReplicator_Activates_AfterParent)
    {
        AZ::Entity childEntity;
        CreateEntityWithChildHierarchy(childEntity);
        SetupEntity(childEntity, NetEntityId{ 2 });

        // we need a parent-id value to be present in NetworkTransformComponent (which is in client mode and doesn't have a controller)
        SetParentIdOnNetworkTransform(childEntity, NetEntityId{ 1 });

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

    /*
     * Parent -> Child
     */
    class SimpleHierarchyTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), NetEntityId{ 2 }, EntityInfo::Role::Child);

            CreateSimpleHierarchy(*m_rootEntityInfo, *m_childEntityInfo);

            m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
            // now the two entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childEntityInfo.reset();
            m_rootEntityInfo.reset();

            StopEntity(*m_childEntity);
            m_childEntity->Deactivate();
            m_childEntity.reset();
            StopEntity(*m_rootEntity);
            m_rootEntity->Deactivate();
            m_rootEntity.reset();

            HierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
    };

    TEST_F(SimpleHierarchyTests, Child_Has_Root_Set)
    {
        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()
        );
    }

    TEST_F(SimpleHierarchyTests, Child_Has_Root_Cleared_On_Detach)
    {
        // now detach the child
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            nullptr
        );
    }

    TEST_F(SimpleHierarchyTests, Root_Has_Child_References)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            1
        );
    }

    TEST_F(SimpleHierarchyTests, Root_Has_Child_References_Removed_On_Detach)
    {
        // now detach the child
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            0
        );
    }

    /*
     * Parent -> Child -> ChildOfChild
     */
    class DeepHierarchyTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");
            m_childOfChildEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(3), "child of child");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), NetEntityId{ 2 }, EntityInfo::Role::Child);
            m_childOfChildEntityInfo = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity.get(), NetEntityId{ 3 }, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_rootEntityInfo, *m_childEntityInfo, *m_childOfChildEntityInfo);

            m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
            m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo.reset();
            m_childEntityInfo.reset();
            m_rootEntityInfo.reset();

            StopEntity(*m_childOfChildEntity);
            m_childOfChildEntity->Deactivate();
            m_childOfChildEntity.reset();
            StopEntity(*m_childEntity);
            m_childEntity->Deactivate();
            m_childEntity.reset();
            StopEntity(*m_rootEntity);
            m_rootEntity->Deactivate();
            m_rootEntity.reset();

            HierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo;
    };

    TEST_F(DeepHierarchyTests, Root_Has_Child_References)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );

        if (m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size() == 2)
        {
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[0],
                m_childEntity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[1],
                m_childOfChildEntity.get()
            );
        }
    }

    TEST_F(DeepHierarchyTests, Root_Has_Child_Of_Child_Reference_Removed_On_Detach)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            1
        );
    }

    TEST_F(DeepHierarchyTests, Root_Has_All_References_Removed_On_Detach_Of_Mid_Child)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            0
        );
    }

    TEST_F(DeepHierarchyTests, Root_Has_All_References_If_Mid_Child_Added_With_Child)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );
    }

    TEST_F(DeepHierarchyTests, Root_Has_All_References_If_Child_Of_Child_Added)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );
    }

    TEST_F(DeepHierarchyTests, Child_Of_Child_Points_To_Root_After_Attach)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()
        );
    }

    TEST_F(DeepHierarchyTests, All_New_Children_Point_To_Root_If_Mid_Child_Added_With_Child)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // reconnect
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()
        );
        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()
        );
    }

    TEST_F(DeepHierarchyTests, Children_Clear_Reference_To_Root_After_Mid_Child_Detached)
    {
        m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_childEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            nullptr
        );
        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            nullptr
        );
    }

    TEST_F(DeepHierarchyTests, Child_Of_Child_Clears_Reference_To_Root_After_Detached)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_childOfChildEntity->FindComponent<NetworkHierarchyChildComponent>()->GetHierarchyRoot(),
            nullptr
        );
    }

    /*
     * Sets up 2 deep hierarchies.
     */
    class HierarchyOfHierarchyTests : public DeepHierarchyTests
    {
    public:
        void SetUp() override
        {
            DeepHierarchyTests::SetUp();

            m_rootEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(4), "root 2");
            m_childEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(5), "child 2");
            m_childOfChildEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(6), "child of child 2");

            m_rootEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_rootEntity2.get(), NetEntityId{ 4 }, EntityInfo::Role::Root);
            m_childEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childEntity2.get(), NetEntityId{ 5 }, EntityInfo::Role::Child);
            m_childOfChildEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity2.get(), NetEntityId{ 6 }, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_rootEntityInfo2, *m_childEntityInfo2, *m_childOfChildEntityInfo2);

            m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity2->GetId());
            m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity2->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo2.reset();
            m_childEntityInfo2.reset();
            m_rootEntityInfo2.reset();

            StopEntity(*m_childOfChildEntity2);
            m_childOfChildEntity2->Deactivate();
            m_childOfChildEntity2.reset();
            StopEntity(*m_childEntity2);
            m_childEntity2->Deactivate();
            m_childEntity2.reset();
            StopEntity(*m_rootEntity2);
            m_rootEntity2->Deactivate();
            m_rootEntity2.reset();

            DeepHierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity2;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo2;
    };

    TEST_F(HierarchyOfHierarchyTests, Hierarchies_Are_Not_Related)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );

        if (m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size() == 2)
        {
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[0],
                m_childEntity.get()
            );
            EXPECT_EQ(
                m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[1],
                m_childOfChildEntity.get()
            );
        }

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );

        if (m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size() == 2)
        {
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[0],
                m_childEntity2.get()
            );
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[1],
                m_childOfChildEntity2.get()
            );
        }
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Root)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_References_All_When_Another_Hierarchy_Attached_At_Child_Of_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Root)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsAttachedToAnotherHierarchy(),
            true
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            m_rootEntity.get()
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsAttachedToAnotherHierarchy(),
            true
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            m_rootEntity.get()
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Inner_Root_References_Top_Root_When_Another_Hierarchy_Attached_At_Child_Of_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->IsAttachedToAnotherHierarchy(),
            true
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyRoot(),
            m_rootEntity.get()
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Inner_Root_Doesnt_Keep_Child_References)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            0
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Detachment_From_Top_Root)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
        // detach
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );
        if (m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size() == 2)
        {
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[0],
                m_childEntity2.get()
            );
            EXPECT_EQ(
                m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren()[1],
                m_childOfChildEntity2.get()
            );
        }
    }

    TEST_F(HierarchyOfHierarchyTests, Inner_Root_Has_Child_References_After_Detachment_From_Child_Of_Child)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Stress_Test_Inner_Root_Has_Child_References_After_Detachment_From_Child_Of_Child)
    {
        for (int i = 0; i < 100; ++i)
        {
            m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
            // detach
            m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        }

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Detachment_Of_Child_Of_Child_In_Inner_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3 - 1
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Attachment_Of_Child_Of_Child_In_Inner_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        // re-connect
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity2->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Child_Of_Child_Changed_Hierarchies)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        // connect to a different hierarchy
        m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Detachment_Of_Child_In_Inner_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3 - 2
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Top_Root_Updates_Child_References_After_Child_Changed_Hierarchies)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());
        // detach
        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        // connect to a different hierarchy
        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2 + 3
        );
    }

    TEST_F(HierarchyOfHierarchyTests, Inner_Root_Has_No_Child_References_After_All_Children_Moved_To_Another_Hierarchy)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childOfChildEntity->GetId());

        m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        // detach
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            0
        );
    }

    /*
     * Parent -> Child -> ChildOfChild (not marked as in a hierarchy)
     */
    class MixedDeepHierarchyTests : public HierarchyTests
    {
    public:
        void SetUp() override
        {
            HierarchyTests::SetUp();

            m_rootEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(1), "root");
            m_childEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(2), "child");
            m_childOfChildEntity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(3), "child of child");

            m_rootEntityInfo = AZStd::make_unique<EntityInfo>(*m_rootEntity.get(), NetEntityId{ 1 }, EntityInfo::Role::Root);
            m_childEntityInfo = AZStd::make_unique<EntityInfo>(*m_childEntity.get(), NetEntityId{ 2 }, EntityInfo::Role::Child);
            m_childOfChildEntityInfo = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity.get(), NetEntityId{ 3 }, EntityInfo::Role::None);

            CreateDeepHierarchy(*m_rootEntityInfo, *m_childEntityInfo, *m_childOfChildEntityInfo);

            m_childEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());
            m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo.reset();
            m_childEntityInfo.reset();
            m_rootEntityInfo.reset();

            StopEntity(*m_childOfChildEntity);
            m_childOfChildEntity->Deactivate();
            m_childOfChildEntity.reset();
            StopEntity(*m_childEntity);
            m_childEntity->Deactivate();
            m_childEntity.reset();
            StopEntity(*m_rootEntity);
            m_rootEntity->Deactivate();
            m_rootEntity.reset();

            HierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity;
        AZStd::unique_ptr<AZ::Entity> m_childEntity;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo;
    };

    TEST_F(MixedDeepHierarchyTests, Top_Root_Ignores_Non_Hierarchical_Entities)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            1
        );
    }

    TEST_F(MixedDeepHierarchyTests, Detaching_Non_Hierarchical_Entity_Has_No_Effect_On_Top_Root)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            1
        );
    }

    TEST_F(MixedDeepHierarchyTests, Attaching_Non_Hierarchical_Entity_Has_No_Effect_On_Top_Root)
    {
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(AZ::EntityId());
        m_childOfChildEntity->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            1
        );
    }

    /*
     * 1st hierarchy: Parent -> Child -> ChildOfChild (not marked as in a hierarchy)
     * 2nd hierarchy: Parent2 -> Child2 (not marked as in a hierarchy) -> ChildOfChild2
     */
    class MixedHierarchyOfHierarchyTests : public MixedDeepHierarchyTests
    {
    public:
        void SetUp() override
        {
            MixedDeepHierarchyTests::SetUp();

            m_rootEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(4), "root 2");
            m_childEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(5), "child 2");
            m_childOfChildEntity2 = AZStd::make_unique<AZ::Entity>(AZ::EntityId(6), "child of child 2");

            m_rootEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_rootEntity2.get(), NetEntityId{ 4 }, EntityInfo::Role::Root);
            m_childEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childEntity2.get(), NetEntityId{ 5 }, EntityInfo::Role::None);
            m_childOfChildEntityInfo2 = AZStd::make_unique<EntityInfo>(*m_childOfChildEntity2.get(), NetEntityId{ 6 }, EntityInfo::Role::Child);

            CreateDeepHierarchy(*m_rootEntityInfo2, *m_childEntityInfo2, *m_childOfChildEntityInfo2);

            m_childEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity2->GetId());
            m_childOfChildEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_childEntity2->GetId());
            // now the entities are under one hierarchy
        }

        void TearDown() override
        {
            m_childOfChildEntityInfo2.reset();
            m_childEntityInfo2.reset();
            m_rootEntityInfo2.reset();

            StopEntity(*m_childOfChildEntity2);
            m_childOfChildEntity2->Deactivate();
            m_childOfChildEntity2.reset();
            StopEntity(*m_childEntity2);
            m_childEntity2->Deactivate();
            m_childEntity2.reset();
            StopEntity(*m_rootEntity2);
            m_rootEntity2->Deactivate();
            m_rootEntity2.reset();

            MixedDeepHierarchyTests::TearDown();
        }

        AZStd::unique_ptr<AZ::Entity> m_rootEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childEntity2;
        AZStd::unique_ptr<AZ::Entity> m_childOfChildEntity2;

        AZStd::unique_ptr<EntityInfo> m_rootEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childEntityInfo2;
        AZStd::unique_ptr<EntityInfo> m_childOfChildEntityInfo2;
    };

    TEST_F(MixedHierarchyOfHierarchyTests, Sanity_Check_Ingore_Children_Without_Hierarchy_Components)
    {
        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            1
        );
        EXPECT_EQ(
            m_rootEntity2->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            0
        );
    }

    TEST_F(MixedHierarchyOfHierarchyTests, Adding_Mixed_Hierarchy_Ingore_Children_Without_Hierarchy_Components)
    {
        m_rootEntity2->FindComponent<AzFramework::TransformComponent>()->SetParent(m_rootEntity->GetId());

        EXPECT_EQ(
            m_rootEntity->FindComponent<NetworkHierarchyRootComponent>()->GetHierarchyChildren().size(),
            2
        );
    }
}
