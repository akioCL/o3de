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
 
 #if !defined(Q_MOC_RUN)
 #include <AzCore/Asset/AssetCommon.h>
 #include <AzCore/Component/TickBus.h>
 #include <AzCore/UserSettings/UserSettings.h>
 #include <AzFramework/Asset/AssetCatalogBus.h>
 #include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
 #include <AzQtComponents/Components/Widgets/TabWidget.h>
 
 #include <QWidget>
 #include <QTimer>
 #endif
 
 namespace AZ
 {
     class SerializeContext;
 }
 
 namespace Ui
 {
     class AssetEditorToolbar;
     class AssetEditorStatusBar;
     class AssetEditorHeader;
 }
 
 namespace AzToolsFramework
 {
     class ReflectedPropertyEditor;
 
     namespace AssetEditor
     {
         class AssetEditorWidget;
 
         /**
         * Provides ability to create, edit, and save reflected assets.
         */
         class AssetEditorTab
             : public QWidget
             , private AZ::Data::AssetBus::Handler
             , private AzFramework::AssetCatalogEventBus::Handler
             , private AzToolsFramework::IPropertyEditorNotify
             , private AZ::SystemTickBus::Handler
         {
             Q_OBJECT
 
         public:
             AZ_CLASS_ALLOCATOR(AssetEditorTab, AZ::SystemAllocator, 0);
 
             using SaveCompleteCallback = AZStd::function<void()>;
 
             explicit AssetEditorTab(QWidget* parent = nullptr);
             ~AssetEditorTab() override;
 
             void LoadAsset(AZ::Data::AssetId assetId, AZ::Data::AssetType assetType, const QString& assetName);
             void CreateAsset(AZ::Data::AssetType assetType, const QString& assetName);
 
             const AZ::Data::AssetId& GetAssetId() const;
             void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
             void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
 
             bool IsDirty() const;
             bool WaitingToSave() const;
 
             const QString& GetAssetName();
 
             bool TrySaveAsset(const AZStd::function<void()>& savedCallback);
             bool UserRefusedSave();
             void ClearUserRefusedSaveFlag();
         public Q_SLOTS:
 
             void SaveAsset();
             bool SaveAsDialog();
             bool SaveAssetToPath(AZStd::string_view assetPath);
             void ExpandAll();
             void CollapseAll();
 
         Q_SIGNALS:
             void OnAssetSavedSignal();
             void OnAssetSaveFailedSignal(const AZStd::string& error);
             void OnAssetRevertedSignal();
             void OnAssetRevertFailedSignal(const AZStd::string& error);
             void OnAssetOpenedSignal(const AZ::Data::Asset<AZ::Data::AssetData>& asset);
 
         protected: // IPropertyEditorNotify
             void AfterPropertyModified(InstanceDataNode* /*node*/) override;
             void RequestPropertyContextMenu(InstanceDataNode*, const QPoint&) override;
 
             void BeforePropertyModified(InstanceDataNode* node) override;
 
             void SetPropertyEditingActive(InstanceDataNode* /*node*/) override {}
             void SetPropertyEditingComplete(InstanceDataNode* /*node*/) override {}
             void SealUndoStack() override {};
 
             void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
             void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
 
         private:
             void DirtyAsset();
 
             void SetStatusText(const QString& assetStatus);
             void ApplyStatusText();
             void SetupHeader();
 
             void SaveAssetImpl(const AZStd::function<void()>& savedCallback);
 
             QString m_queuedAssetStatus;
 
             // AZ::SystemTickBus
             void OnSystemTick() override;
 
             AZ::Data::AssetId                    m_sourceAssetId;
             AZ::Data::Asset<AZ::Data::AssetData> m_inMemoryAsset;
             Ui::AssetEditorHeader* m_header;
             ReflectedPropertyEditor* m_propertyEditor;
             AZ::SerializeContext* m_serializeContext = nullptr;
 
             // Ids can change when an asset goes from in-memory to saved on disk.
             // If there is a failure, the asset will be removed from the catalog.
             // The only reliable mechanism to be certain the asset being added/removed 
             // from the catalog is the same one that was added is to compare its file path.
             AZStd::string m_expectedAddedAssetPath; 
             AZStd::string m_recentlyAddedAssetPath;
 
             bool m_dirty = false;
             bool m_userRefusedSave = false;
             
             QString m_currentAsset;
 
             AssetEditorWidget* m_parentEditorWidget = nullptr;
 
             AZStd::vector<AZ::u8> m_saveData;
             bool m_closeTabAfterSave = false;
             bool m_closeParentAfterSave = false;
             bool m_waitingToSave = false;
 
             void CreateAssetImpl(AZ::Data::AssetType assetType, const QString& assetName);
             bool SaveImpl(const AZ::Data::Asset<AZ::Data::AssetData>& asset, const QString& saveAsPath);
             void UpdatePropertyEditor(AZ::Data::Asset<AZ::Data::AssetData>& asset);
 
             void GenerateSaveDataSnapshot();
 
             QIcon m_warningIcon;
         };
     } // namespace AssetEditor
 } // namespace AzToolsFramework
