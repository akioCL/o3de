/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/PrefabEditManager.h>

#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        PrefabEditManager::PrefabEditManager()
        {
            m_prefabPublicInterface = AZ::Interface<PrefabPublicInterface>::Get();

            if (m_prefabPublicInterface == nullptr)
            {
                AZ_Assert(false, "Prefab - could not get PrefabPublicInterface on PrefabEditManager construction.");
                return;
            }

            AZ::Interface<PrefabEditInterface>::Register(this);
            AZ::Interface<EditorInteractionInterface>::Register(this);
        }

        PrefabEditManager::~PrefabEditManager()
        {
            AZ::Interface<EditorInteractionInterface>::Unregister(this);
            AZ::Interface<PrefabEditInterface>::Unregister(this);
        }

        void PrefabEditManager::EditOwningPrefab(AZ::EntityId entityId)
        {
            m_instanceBeingEdited = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
        }

        bool PrefabEditManager::IsOwningPrefabBeingEdited(AZ::EntityId entityId)
        {
            AZ::EntityId containerEntity = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);
            return m_instanceBeingEdited == containerEntity;
        }

        AZ::EntityId PrefabEditManager::RedirectEntitySelection(AZ::EntityId entityId)
        {
            // Retrieve the Level Container
            AZ::EntityId levelContainerEntityId = m_prefabPublicInterface->GetLevelInstanceContainerEntityId();

            // Find container entity for owning prefab of passed entity
            AZ::EntityId containerEntityId = m_prefabPublicInterface->GetInstanceContainerEntityId(entityId);

            // If the entity belongs to the level instance or an instance that is currently being edited, it can be selected
            if (containerEntityId == levelContainerEntityId || m_editedPrefabHierarchyCache.contains(containerEntityId))
            {
                return entityId;
            }

            // Else keep looping until you can find an instance that is being edited, or the level instance
            AZ::EntityId parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);

            while (parentContainerEntityId.IsValid() && parentContainerEntityId != levelContainerEntityId &&
                   !m_editedPrefabHierarchyCache.contains(parentContainerEntityId))
            {
                // Else keep going up the hierarchy
                containerEntityId = parentContainerEntityId;
                parentContainerEntityId = m_prefabPublicInterface->GetParentInstanceContainerEntityId(containerEntityId);
            }

            return containerEntityId;
        }
    }
}
