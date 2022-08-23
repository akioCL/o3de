/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/PrefabInstanceUtils.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabInstanceUtils
        {
            InstanceClimbUpResult ClimbUpToTargetOrRootInstance(const Instance& startInstance, InstanceOptionalConstReference targetInstance)
            {
                InstanceClimbUpResult result;
                
                // Climbs up the instance hierarchy from the start instance until it hits the target or the root instance.
                const Instance* instancePtr = &startInstance;

                while (instancePtr != &(targetInstance->get()) && instancePtr->HasParentInstance())
                {
                    result.m_climbedInstances.emplace_back(*instancePtr);
                    instancePtr = &(instancePtr->GetParentInstance()->get());
                }

                result.m_reachedInstance = instancePtr;
                return result;
            }

            AZStd::string GetRelativePathBetweenInstances(const Instance& parentInstance, const Instance& childInstance)
            {
                const Instance* instancePtr = &childInstance;
                AZStd::vector<InstanceOptionalConstReference> climbedInstances;

                // Climbs up the instance hierarchy from the child instance until it hits the parent instance.
                while (instancePtr != &parentInstance && instancePtr->HasParentInstance())
                {
                    climbedInstances.emplace_back(*instancePtr);
                    instancePtr = &(instancePtr->GetParentInstance()->get());
                }

                if (instancePtr != &parentInstance)
                {
                    AZ_Warning("Prefab", false, "PrefabInstanceUtils::GetRelativePathBetweenInstances() - "
                               "Tried to get relative path but the parent instance is not a valid parent. Returns empty string instead.");
                    return "";
                }

                return GetRelativePathFromClimbedInstances(climbedInstances);
            }

            AZStd::string GetRelativePathFromClimbedInstances(const AZStd::vector<InstanceOptionalConstReference>& climbedInstances)
            {
                AZStd::string relativePath = "";

                for (auto instanceIter = climbedInstances.rbegin(); instanceIter != climbedInstances.rend(); ++instanceIter)
                {
                    relativePath.append(PrefabDomUtils::PathStartingWithInstances);
                    const Instance& instance = (*instanceIter)->get();
                    relativePath.append(instance.GetInstanceAlias());
                }

                return relativePath;
            }

            bool IsDescendantInstance(const Instance& childInstance, const Instance& parentInstance)
            {
                const Instance* instancePtr = &childInstance;

                while (instancePtr)
                {
                    if (instancePtr == &parentInstance)
                    {
                        return true;
                    }
                    instancePtr = instancePtr->HasParentInstance() ? &(instancePtr->GetParentInstance()->get()) : nullptr;
                }

                return false;
            }



            AZStd::pair<const Instance*, AZStd::vector<InstanceOptionalConstReference>> ClimbUpToTargetOrRootInstance_Old(
                const Instance* startInstance, const Instance* targetInstance)
            {
                if (!startInstance)
                {
                    return AZStd::make_pair(nullptr, AZStd::vector<InstanceOptionalConstReference>());
                }

                // Climb up the instance hierarchy from this instance until you hit the target or the root.
                InstanceOptionalConstReference instance = *startInstance;
                AZStd::vector<InstanceOptionalConstReference> instancesInHierarchy;

                while (&instance->get() != targetInstance && instance->get().GetParentInstance() != AZStd::nullopt)
                {
                    instancesInHierarchy.emplace_back(instance);
                    instance = instance->get().GetParentInstance();
                }

                return AZStd::make_pair(&instance->get(), AZStd::move(instancesInHierarchy));
            }

            AZStd::pair<const Instance*, AZStd::string> GetRelativePathBetweenInstances_Old(
                const Instance* startInstance, const Instance* targetInstance)
            {
                if (!startInstance)
                {
                    return AZStd::make_pair(nullptr, "");
                }

                auto climbUpToTargetOrRootInstanceResult = ClimbUpToTargetOrRootInstance(startInstance, targetInstance);

                AZStd::string relativePathToStartInstance;
                const auto& instancePath = climbUpToTargetOrRootInstanceResult.second;
                for (auto instanceIter = instancePath.crbegin(); instanceIter != instancePath.crend(); ++instanceIter)
                {
                    relativePathToStartInstance.append("/Instances/");
                    relativePathToStartInstance.append((*instanceIter)->get().GetInstanceAlias());
                }

                return AZStd::make_pair(climbUpToTargetOrRootInstanceResult.first, AZStd::move(relativePathToStartInstance));
            }

        } // namespace PrefabInstanceUtils
    } // namespace Prefab
} // namespace AzToolsFramework
