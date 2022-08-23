/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/InstanceDomGenerator.h>

#include <AzCore/Interface/Interface.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>
#include <AzToolsFramework/Prefab/PrefabFocusInterface.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        AzFramework::EntityContextId InstanceDomGenerator::s_editorEntityContextId = AzFramework::EntityContextId::CreateNull();

        void InstanceDomGenerator::RegisterInstanceDomGeneratorInterface()
        {
            AZ::Interface<InstanceDomGeneratorInterface>::Register(this);

            // Get EditorEntityContextId
            EditorEntityContextRequestBus::BroadcastResult(s_editorEntityContextId, &EditorEntityContextRequests::GetEditorEntityContextId);

            m_prefabSystemComponentInterface = AZ::Interface<PrefabSystemComponentInterface>::Get();
            AZ_Assert(m_prefabSystemComponentInterface != nullptr,
                "Prefab - InstanceDomGenerator::Initialize - "
                "Prefab System Component Interface could not be found. "
                "Check that it is being correctly initialized.");
        }

        void InstanceDomGenerator::UnregisterInstanceDomGeneratorInterface()
        {
            m_prefabSystemComponentInterface = nullptr;

            AZ::Interface<InstanceDomGeneratorInterface>::Unregister(this);
        }

        bool InstanceDomGenerator::GenerateInstanceDom_Old(const Instance* instance, PrefabDom& instanceDom)
        {
            // Retrieve focused instance.
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            AZ_Assert(prefabFocusInterface, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                "Prefab Focus Interface couldn not be found.");

            InstanceOptionalReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);
            const Instance* targetInstance = nullptr;
            if (focusedInstance.has_value())
            {
                targetInstance = &(focusedInstance->get());
            }

            auto climbUpToDomSourceInstanceResult = PrefabInstanceUtils::GetRelativePathBetweenInstances_Old(instance, targetInstance);
            auto domSourceInstance = climbUpToDomSourceInstanceResult.first;
            AZStd::string& relativePathToDomSourceInstance = climbUpToDomSourceInstanceResult.second;

            PrefabDomPath domSourcePath(relativePathToDomSourceInstance.c_str());
            PrefabDom partialInstanceDom;
            partialInstanceDom.CopyFrom(
                m_prefabSystemComponentInterface->FindTemplateDom(domSourceInstance->GetTemplateId()), instanceDom.GetAllocator());

            auto instanceDomValueFromSource = domSourcePath.Get(partialInstanceDom);
            if (!instanceDomValueFromSource)
            {
                return false;
            }

            instanceDom.CopyFrom(*instanceDomValueFromSource, instanceDom.GetAllocator());

            // If the focused instance is not an ancestor of our instance, verify if it's a descendant.
            if (domSourceInstance != targetInstance)
            {
                auto climbUpToFocusedInstanceAncestorResult =
                    PrefabInstanceUtils::GetRelativePathBetweenInstances_Old(targetInstance, instance);
                auto focusedInstanceAncestor = climbUpToFocusedInstanceAncestorResult.first;
                AZStd::string& relativePathToFocusedInstanceAncestor = climbUpToFocusedInstanceAncestorResult.second;

                // If the focused instance is a descendant (or the instance itself), we need to replace its portion of the dom with the template one.
                if (focusedInstanceAncestor != nullptr && focusedInstanceAncestor == instance)
                {
                    // Get the dom for the focused instance from its template.
                    PrefabDom focusedInstanceDom;
                    focusedInstanceDom.CopyFrom(
                        m_prefabSystemComponentInterface->FindTemplateDom(focusedInstance->get().GetTemplateId()),
                        instanceDom.GetAllocator());

                    // Replace the container entity with the one as seen by the root
                    // TODO - this function should only replace the transform!
                    UpdateContainerEntityInDomFromRoot(focusedInstanceDom, focusedInstance->get());
                    
                    // Copy the focused instance dom inside the dom that will be used to refresh the instance.
                    PrefabDomPath domSourceToFocusPath(relativePathToFocusedInstanceAncestor.c_str());
                    domSourceToFocusPath.Set(instanceDom, focusedInstanceDom, instanceDom.GetAllocator());

                    // Force a deep copy
                    PrefabDom instanceDomCopy;
                    instanceDomCopy.CopyFrom(instanceDom, instanceDom.GetAllocator());

                    instanceDom.CopyFrom(instanceDomCopy, instanceDom.GetAllocator());
                }
            }
            // If our instance is the focused instance, fix the container
            else if (&focusedInstance->get() == instance)
            {
                // Replace the container entity with the one as seen by the root
                // TODO - this function should only replace the transform!
                UpdateContainerEntityInDomFromRoot(instanceDom, *instance);
            }

            PrefabDomValueReference instanceDomFromRoot = instanceDom;
            if (!instanceDomFromRoot.has_value())
            {
                return false;
            }

            return true;
        }

        bool InstanceDomGenerator::GenerateInstanceDom(PrefabDom& instanceDom, const Instance& instance) const
        {
            // Retrieves the focused instance.
            auto prefabFocusInterface = AZ::Interface<PrefabFocusInterface>::Get();
            if (!prefabFocusInterface)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Prefab Focus Interface couldn not be found.");
                return false;
            }

            InstanceOptionalConstReference focusedInstance = prefabFocusInterface->GetFocusedPrefabInstance(s_editorEntityContextId);

            // Climbs up from the given instance to root instance, but stops at the focused instance if they can meet.
            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance, focusedInstance);
            const Instance* focusedOrRootInstancePtr = climbUpResult.m_reachedInstance;
            if (!focusedOrRootInstancePtr) 
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Could not get the focused or root instance. It should not be null.");
                return false;
            }

            // Copies the instance DOM, that is stored in the focused or root template DOM, into the output instance DOM.
            AZStd::string relativePathFromTop = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);
            PrefabDomPath relativeDomPath(relativePathFromTop.c_str());
            PrefabDom focusedOrRootTemplateDom;
            focusedOrRootTemplateDom.CopyFrom(
                m_prefabSystemComponentInterface->FindTemplateDom((*focusedOrRootInstancePtr).GetTemplateId()),
                instanceDom.GetAllocator());
            auto instanceDomFromTemplate = relativeDomPath.Get(focusedOrRootTemplateDom);
            if (!instanceDomFromTemplate)
            {
                AZ_Assert(false, "Prefab - InstanceDomGenerator::GenerateInstanceDom - "
                    "Could not get the instance DOM stored in the focused or root template DOM.");
                return false;
            }

            instanceDom.CopyFrom(*instanceDomFromTemplate, instanceDom.GetAllocator());

            if (!focusedInstance.has_value())
            {
                return true;
            }

            // Additional processing on focused instance DOM...

            // If the focused instance is the given instance, then update the container entity in focused instance DOM
            // (i.e. the given instance DOM) with the one seen by the root.
            if (&instance == &(focusedInstance->get()))
            {
                UpdateContainerEntityInDomFromRoot(instanceDom, focusedInstance->get());
            }
            // If the focused instance is a descendant of the given instance, then update the corresponding
            // portion in the output instance DOM with the focused template DOM. Also, updates the container entity
            // in focused instance DOM with the one seen by the root.
            else if (PrefabInstanceUtils::IsDescendantInstance(focusedInstance->get(), instance))
            {
                PrefabDom focusedTemplateDom;
                focusedTemplateDom.CopyFrom(m_prefabSystemComponentInterface->FindTemplateDom(focusedInstance->get().GetTemplateId()),
                    instanceDom.GetAllocator());

                UpdateContainerEntityInDomFromRoot(focusedTemplateDom, focusedInstance->get());

                // Forces a hard copy using the instanceDom's allocator.
                PrefabDom focusedTemplateDomCopy;
                focusedTemplateDomCopy.CopyFrom(focusedTemplateDom, instanceDom.GetAllocator());

                // Stores the focused DOM into the instance DOM.
                AZStd::string relativePathToFocus = PrefabInstanceUtils::GetRelativePathBetweenInstances(
                    instance, focusedInstance->get());
                PrefabDomPath relativeDomPathToFocus(relativePathToFocus.c_str());

                relativeDomPathToFocus.Set(instanceDom, focusedTemplateDomCopy, instanceDom.GetAllocator());
            }
            // Skips additional processing if the focused instance is a proper ancestor of the given instance, or
            // the focused instance has no hierarchy relation with the given instance.

            return true;
        }

        void InstanceDomGenerator::UpdateContainerEntityInDomFromRoot(PrefabDom& instanceDom, const Instance& instance) const
        {
            // TODO: Modifies the function so it updates the transform only.

            InstanceClimbUpResult climbUpResult = PrefabInstanceUtils::ClimbUpToTargetOrRootInstance(instance);
            const Instance* rootInstancePtr = climbUpResult.m_reachedInstance;
            AZStd::string relativePathFromRoot = PrefabInstanceUtils::GetRelativePathFromClimbedInstances(climbUpResult.m_climbedInstances);

            // No need to update the instance DOM if the given instance is the root instance.
            if (rootInstancePtr == &instance)
            {
                return;
            }

            // Creates a path from the root instance to container entity of the given insance.
            const AZStd::string pathToContainerEntity =
                AZStd::string::format("%s/%s", relativePathFromRoot.c_str(), PrefabDomUtils::ContainerEntityName);
            PrefabDomPath domPathToContainerEntity(pathToContainerEntity.c_str());

            // Retrieves the container entity DOM of the root template DOM.
            const PrefabDom& rootTemplateDom = m_prefabSystemComponentInterface->FindTemplateDom((*rootInstancePtr).GetTemplateId());
            PrefabDom containerEntityDom;
            containerEntityDom.CopyFrom(*domPathToContainerEntity.Get(rootTemplateDom), instanceDom.GetAllocator());

            // Sets the container entity DOM in the given instance DOM.
            const AZStd::string containerEntityName = AZStd::string::format("/%s", PrefabDomUtils::ContainerEntityName);
            PrefabDomPath containerEntityPath(containerEntityName.c_str());
            containerEntityPath.Set(instanceDom, containerEntityDom, instanceDom.GetAllocator());
        }

        void InstanceDomGenerator::ReplaceFocusedContainerTransformAccordingToRoot(
            const Instance* focusedInstance, PrefabDom& focusedInstanceDom) const
        {
            // Climb from the focused instance to the root and store the path.
            auto climbUpToFocusedInstanceResult = PrefabInstanceUtils::GetRelativePathBetweenInstances_Old(focusedInstance, nullptr);
            auto rootInstance = climbUpToFocusedInstanceResult.first;
            AZStd::string& rootToFocusedInstance = climbUpToFocusedInstanceResult.second;

            if (rootInstance != focusedInstance)
            {
                // Create the path from the root instance to the container entity of the focused instance.
                AZStd::string rootToFocusedInstanceContainer =
                    AZStd::string::format("%s/%s", rootToFocusedInstance.c_str(), PrefabDomUtils::ContainerEntityName);
                PrefabDomPath rootToFocusedInstanceContainerPath(rootToFocusedInstanceContainer.c_str());

                // Retrieve the dom of the root instance.
                PrefabDom rootDom;
                rootDom.CopyFrom(
                    m_prefabSystemComponentInterface->FindTemplateDom(rootInstance->GetTemplateId()), focusedInstanceDom.GetAllocator());

                PrefabDom containerDom;
                containerDom.CopyFrom(*rootToFocusedInstanceContainerPath.Get(rootDom), focusedInstanceDom.GetAllocator());

                // Paste the focused instance container dom as seen from the root into instanceDom.
                AZStd::string containerName =
                    AZStd::string::format("/%s", PrefabDomUtils::ContainerEntityName);
                PrefabDomPath containerPath(containerName.c_str());
                containerPath.Set(focusedInstanceDom, containerDom, focusedInstanceDom.GetAllocator());
            }
        }
    } // namespace Prefab
} // namespace AzToolsFramework
