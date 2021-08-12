/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::Serialization
{
    class DataElementNode;


    // Types listed earlier here will have higher priority
    enum class DataPatchUpgradeType
    {
        TYPE_UPGRADE,
        NAME_UPGRADE
    };

    // Base type used for single-node version upgrades
    class DataPatchUpgrade
    {
    public:
        AZ_CLASS_ALLOCATOR(DataPatchUpgrade, SystemAllocator, 0);
        AZ_RTTI(DataPatchUpgrade, "{FD1C3109-0883-45FF-A12F-CAF5E323E954}");

        DataPatchUpgrade(AZStd::string_view fieldName, unsigned int fromVersion, unsigned int toVersion);
        virtual ~DataPatchUpgrade() = default;

        // This will only check to see if this has the same upgrade type and is applied to the same field.
        // Deeper comparisons are left to the individual upgrade types.
        bool operator==(const DataPatchUpgrade& RHS) const;

        // Used to sort nodes upgrades.
        bool operator<(const DataPatchUpgrade& RHS) const;

        virtual void Apply(AZ::SerializeContext& /*context*/, DataElementNode& /*elementNode*/) const {}

        unsigned int FromVersion() const;
        unsigned int ToVersion() const;

        const AZStd::string& GetFieldName() const;
        AZ::Crc32 GetFieldCRC() const;

        DataPatchUpgradeType GetUpgradeType() const;

        // Type Upgrade Interface Functions
        virtual AZ::TypeId GetFromType() const { return AZ::TypeId::CreateNull(); }
        virtual AZ::TypeId GetToType() const { return AZ::TypeId::CreateNull(); }

        virtual AZStd::any Apply(const AZStd::any& in) const { return in; }

        // Name upgrade interface functions
        virtual AZStd::string GetNewName() const { return {}; }

    protected:
        AZStd::string m_targetFieldName;
        AZ::Crc32 m_targetFieldCRC;
        unsigned int m_fromVersion;
        unsigned int m_toVersion;

        DataPatchUpgradeType m_upgradeType;
    };

    // Binary predicate for ordering per-version upgrades
    // When multiple upgrades exist from a particular version, we only want
    // to apply the one that upgrades to the maximum possible version.
    struct NodeUpgradeSortFunctor
    {
        // Provides sorting of lists of node upgrade pointers
        bool operator()(const DataPatchUpgrade* LHS, const DataPatchUpgrade* RHS)
        {
            if (LHS->ToVersion() == RHS->ToVersion())
            {
                AZ_Assert(LHS->GetUpgradeType() != RHS->GetUpgradeType(), "Data Patch Upgrades with the same from/to version numbers must be different types.");

                if (LHS->GetUpgradeType() == DataPatchUpgradeType::NAME_UPGRADE)
                {
                    return true;
                }

                return false;
            }
            return LHS->ToVersion() < RHS->ToVersion();
        }
    };

    // A list of node upgrades, sorted in order of the version they convert to
    using DataPatchUpgradeList = AZStd::set<DataPatchUpgrade*, NodeUpgradeSortFunctor>;
    // A sorted mapping of upgrade lists, keyed by (and sorted on) the version they convert from
    using DataPatchUpgradeMap = AZStd::map<unsigned int, DataPatchUpgradeList>;
    // Stores sets of node upgrades keyed by the field they apply to
    using DataPatchFieldUpgrades = AZStd::unordered_map<AZ::Crc32, DataPatchUpgradeMap>;

    // A class to maintain and apply all of the per-field node upgrades that apply to one single field.
    // Performs error checking when building the field array, manages the lifetime of the upgrades, and
    // deals with application of the upgrades to both nodes and raw values.
    class DataPatchUpgradeHandler
    {
    public:
        DataPatchUpgradeHandler()
        {}

        ~DataPatchUpgradeHandler();

        // This function assumes ownership of the upgrade
        void AddFieldUpgrade(DataPatchUpgrade* upgrade);

        const DataPatchFieldUpgrades& GetUpgrades() const;

    private:
        DataPatchFieldUpgrades m_upgrades;
    };

    class DataPatchNameUpgrade : public DataPatchUpgrade
    {
    public:
        AZ_CLASS_ALLOCATOR(DataPatchNameUpgrade, SystemAllocator, 0);
        AZ_RTTI(DataPatchNameUpgrade, "{9991F242-7D3B-4E76-B3EA-2E09AE14187D}", DataPatchUpgrade);

        DataPatchNameUpgrade(unsigned int fromVersion, unsigned int toVersion, AZStd::string_view oldName, AZStd::string_view newName)
            : DataPatchUpgrade(oldName, fromVersion, toVersion)
            , m_newNodeName(newName)
        {
            m_upgradeType = DataPatchUpgradeType::NAME_UPGRADE;
        }

        ~DataPatchNameUpgrade() override = default;

        // The equivalence operator is used to determine if upgrades are functionally equivalent.
        // Two upgrades are considered equivalent (Incompatible) if they operate on the same field,
        // are the same type of upgrade, and upgrade from the same version.
        bool operator<(const DataPatchUpgrade& RHS) const;
        bool operator<(const DataPatchNameUpgrade& RHS) const;

        void Apply(AZ::SerializeContext& context, DataElementNode& node) const override;
        using DataPatchUpgrade::Apply;

        AZStd::string GetNewName() const override;

    private:
        AZStd::string m_newNodeName;
    };

    template<class FromT, class ToT>
    class DataPatchTypeUpgrade : public DataPatchUpgrade
    {
    public:
        AZ_CLASS_ALLOCATOR(DataPatchTypeUpgrade, SystemAllocator, 0);
        AZ_RTTI(DataPatchTypeUpgrade, "{E5A2F519-261C-4B81-925F-3730D363AB9C}", DataPatchUpgrade);

        DataPatchTypeUpgrade(AZStd::string_view nodeName, unsigned int fromVersion, unsigned int toVersion, AZStd::function<ToT(const FromT& data)> upgradeFunc)
            : DataPatchUpgrade(nodeName, fromVersion, toVersion)
            , m_upgrade(AZStd::move(upgradeFunc))
            , m_fromTypeID(azrtti_typeid<FromT>())
            , m_toTypeID(azrtti_typeid<ToT>())
        {
            m_upgradeType = DataPatchUpgradeType::TYPE_UPGRADE;
        }

        ~DataPatchTypeUpgrade() override = default;

        bool operator<(const DataPatchTypeUpgrade& RHS) const
        {
            return DataPatchUpgrade::operator<(RHS);
        }

        AZStd::any Apply(const AZStd::any& in) const override
        {
            const FromT& inValue = AZStd::any_cast<const FromT&>(in);
            return AZStd::any(m_upgrade(inValue));
        }

        void Apply(AZ::SerializeContext& context, SerializeContext::DataElementNode& node) const override
        {
            auto targetElementIndex = node.FindElement(m_targetFieldCRC);

            AZ_Assert(targetElementIndex >= 0, "Invalid node. Field %s is not a valid element of class %s (Version %d). Check your reflection function.", m_targetFieldName.data(), node.GetNameString(), node.GetVersion());

            if (targetElementIndex >= 0)
            {
                AZ::SerializeContext::DataElementNode& targetElement = node.GetSubElement(targetElementIndex);

                // We're replacing the target node so store the name off for use later
                const char* targetElementName = targetElement.GetNameString();

                FromT fromData;

                // Get the current value at the target node
                bool success = targetElement.GetData<FromT>(fromData);

                AZ_Assert(success, "A single node type conversion of class %s (version %d) failed on field %s. The original node exists but isn't the correct type. Check your class reflection.", node.GetNameString(), node.GetVersion(), targetElementName);

                if (success)
                {
                    node.RemoveElement(targetElementIndex);

                    // Apply the user's provided data converter function
                    ToT toData = m_upgrade(fromData);

                    // Add the converted data back into the node as a new element with the same name
                    auto newIndex = node.AddElement<ToT>(context, targetElementName);
                    auto& newElement = node.GetSubElement(newIndex);
                    newElement.SetData(context, toData);
                }
            }
        }

        AZ::TypeId GetFromType() const override
        {
            return m_fromTypeID;
        }

        AZ::TypeId GetToType() const override
        {
            return m_toTypeID;
        }

    private:
        AZStd::function<ToT(const FromT& data)> m_upgrade;

        // Used for comparison of upgrade functions to determine uniqueness
        AZ::TypeId m_fromTypeID;
        AZ::TypeId m_toTypeID;
    };

}

