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

#pragma once

#include <NetworkEntity/NetworkEntityTracker.h>

#include <AzCore/EBus/Event.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzNetworking/DataStructures/TimeoutQueue.h>

namespace Multiplayer
{
    class INetworkEntityManager;

    class NetworkEntityAuthorityTracker
    {
    public:
        NetworkEntityAuthorityTracker(INetworkEntityManager& networkEntityManager);

        bool DoesEntityHaveOwner(ConstNetworkEntityHandle entityHandle) const;
        bool AddEntityAuthorityManager(ConstNetworkEntityHandle entityHandle, HostId newOwner);
        void RemoveEntityAuthorityManager(ConstNetworkEntityHandle entityHandle, HostId previousOwner);
        HostId GetEntityAuthorityManager(ConstNetworkEntityHandle entityHandle) const;

    private:

        HostId GetEntityAuthorityManagerInternal(ConstNetworkEntityHandle entityHandle) const;

        NetworkEntityAuthorityTracker& operator= (const NetworkEntityAuthorityTracker&) = delete;

        struct TimeoutData final
        {
            TimeoutData() = default;
            TimeoutData(ConstNetworkEntityHandle entityHandle, HostId previousOwner);
            ConstNetworkEntityHandle m_entityHandle;
            HostId m_previousOwner = InvalidHostId;
        };

        struct NetworkEntityTimeoutFunctor final
            : public AzNetworking::ITimeoutHandler
        {
            NetworkEntityTimeoutFunctor(NetworkEntityAuthorityTracker& networkEntityAuthorityTracker, INetworkEntityManager& m_networkEntityManager);
            AzNetworking::TimeoutResult HandleTimeout(AzNetworking::TimeoutQueue::TimeoutItem& item) override;
        private:
            AZ_DISABLE_COPY_MOVE(NetworkEntityTimeoutFunctor);
            NetworkEntityAuthorityTracker& m_networkEntityAuthorityTracker;
            INetworkEntityManager& m_networkEntityManager;
        };

        using TimeoutDataMap = AZStd::unordered_map<NetEntityId, TimeoutData>;
        using EntityAuthorityMap = AZStd::unordered_map<NetEntityId, AZStd::vector<HostId>>;

        TimeoutDataMap m_timeoutDataMap;
        EntityAuthorityMap m_entityAuthorityMap;
        INetworkEntityManager& m_networkEntityManager;
        AzNetworking::TimeoutQueue m_timeoutQueue;
    };
}

