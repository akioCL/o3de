/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/DataPatch.h>
#include <AzCore/std/containers/set.h>
#include <AzCore/std/containers/map.h>

namespace AZ
{
    namespace Serialization
    {
        class DataPatchUpgrade;

        struct NodeUpgradeSortFunctor;
        using DataPatchUpgradeList = AZStd::set<DataPatchUpgrade*, NodeUpgradeSortFunctor>;
        using DataPatchUpgradeMap = AZStd::map<unsigned int, DataPatchUpgradeList>;
        using DataPatchFieldUpgrades = AZStd::unordered_map<AZ::Crc32, DataPatchUpgradeMap>;
    }

    // Manages a collection of data patch upgrades
    class DataPatchUpgradeManager
    {
    public:
        // Upgrade a data patch. The address elements of the data patch must have type and version information. Patches created prior to the implementation of
        // the type change and name change class builders will not be upgraded. In addition, there must be TypeChange and/or NameChange class builders specified
        // that apply to the given patch.
        static void UpgradeDataPatch(AZ::SerializeContext* context, AZ::TypeId targetClassID, unsigned int targetClassVersion, DataPatch::AddressType& address, AZStd::any& data);

    private:
        // This class requires field upgrades from the serialize context to function.
        DataPatchUpgradeManager(const AZ::Serialization::DataPatchFieldUpgrades& upgrades, int startingVersion);

        // Apply appropriate upgrades to an address. Returns a new address.
        void ApplyUpgrades(AZ::DataPatch::AddressTypeElement& addressElement, AZStd::vector<AZ::Serialization::DataPatchUpgrade*> upgrades = AZStd::vector<AZ::Serialization::DataPatchUpgrade*>()) const;

        // Apply appropriate upgrades to a value AND an address. Returns a new value.
        void ApplyUpgrades(AZ::DataPatch::AddressTypeElement& addressElement, AZStd::any& value) const;

        // Retrieve an ordered list of upgrades managed by this handler that should be applied
        // to the given address or an associated value. Returns an empty vector if there are none.
        AZStd::vector<AZ::Serialization::DataPatchUpgrade*> GetApplicableUpgrades(const DataPatch::AddressTypeElement& address) const;

        // Helper Functions
        const AZ::Serialization::DataPatchUpgradeMap* GetPotentialUpgrades(const AZ::Crc32& fieldNameCRC) const;
        AZ::Serialization::DataPatchUpgrade* GetNextUpgrade(unsigned int currentTypeVersion, unsigned int currentNameVersion, AZ::Crc32 currentFieldNameCRC) const;
        AZ::Serialization::DataPatchUpgrade* GetNextTypeUpgrade(AZ::Crc32 currentFieldNameCRC, unsigned int currentTypeVersion) const;
        AZ::Serialization::DataPatchUpgrade* GetNextNameUpgrade(AZ::Crc32 currentFieldNameCRC, unsigned int currentNameVersion) const;

    private:
        const AZ::Serialization::DataPatchFieldUpgrades& m_upgrades;
        unsigned int m_startingVersion;
    };
}
