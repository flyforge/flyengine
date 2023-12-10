#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Assets/AssetCurator.h>

plActionDescriptorHandle plAssetActions::s_hAssetCategory;
plActionDescriptorHandle plAssetActions::s_hTransformAsset;
plActionDescriptorHandle plAssetActions::s_hTransformAllAssets;
plActionDescriptorHandle plAssetActions::s_hResaveAllAssets;
plActionDescriptorHandle plAssetActions::s_hCheckFileSystem;
plActionDescriptorHandle plAssetActions::s_hWriteLookupTable;
plActionDescriptorHandle plAssetActions::s_hWriteDependencyDGML;

void plAssetActions::RegisterActions()
{
  s_hAssetCategory = PLASMA_REGISTER_CATEGORY("AssetCategory");
  s_hTransformAsset = PLASMA_REGISTER_ACTION_1("Asset.Transform", plActionScope::Document, "Assets", "Ctrl+E", plAssetAction, plAssetAction::ButtonType::TransformAsset);
  s_hTransformAllAssets = PLASMA_REGISTER_ACTION_1("Asset.TransformAll", plActionScope::Global, "Assets", "Ctrl+Shift+E", plAssetAction, plAssetAction::ButtonType::TransformAllAssets);
  s_hResaveAllAssets = PLASMA_REGISTER_ACTION_1("Asset.ResaveAll", plActionScope::Global, "Assets", "", plAssetAction, plAssetAction::ButtonType::ResaveAllAssets);
  s_hCheckFileSystem = PLASMA_REGISTER_ACTION_1("Asset.CheckFilesystem", plActionScope::Global, "Assets", "", plAssetAction, plAssetAction::ButtonType::CheckFileSystem);
  s_hWriteLookupTable = PLASMA_REGISTER_ACTION_1("Asset.WriteLookupTable", plActionScope::Global, "Assets", "", plAssetAction, plAssetAction::ButtonType::WriteLookupTable);
  s_hWriteDependencyDGML = PLASMA_REGISTER_ACTION_1("Asset.WriteDependencyDGML", plActionScope::Document, "Assets", "", plAssetAction, plAssetAction::ButtonType::WriteDependencyDGML);
}

void plAssetActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hAssetCategory);
  plActionManager::UnregisterAction(s_hTransformAsset);
  plActionManager::UnregisterAction(s_hTransformAllAssets);
  plActionManager::UnregisterAction(s_hResaveAllAssets);
  plActionManager::UnregisterAction(s_hCheckFileSystem);
  plActionManager::UnregisterAction(s_hWriteLookupTable);
  plActionManager::UnregisterAction(s_hWriteDependencyDGML);
}

void plAssetActions::MapMenuActions(plStringView sMapping)
{
  const plStringView sTargetMenu = "G.AssetDoc";

  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", sMapping);

  pMap->MapAction(s_hTransformAsset, sTargetMenu, 1.0f);
  pMap->MapAction(s_hWriteDependencyDGML, sTargetMenu, 10.0f);
}

void plAssetActions::MapToolBarActions(plStringView sMapping, bool bDocument)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hAssetCategory, "", 10.0f);

  if (bDocument)
  {
    pMap->MapAction(s_hTransformAsset, "AssetCategory", 1.0f);
  }
  else
  {
    pMap->MapAction(s_hCheckFileSystem, "AssetCategory", 1.0f);
    pMap->MapAction(s_hTransformAllAssets, "AssetCategory", 2.0f);
    pMap->MapAction(s_hResaveAllAssets, "AssetCategory", 3.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// plAssetAction
////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAssetAction::plAssetAction(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case plAssetAction::ButtonType::TransformAsset:
      SetIconPath(":/EditorFramework/Icons/TransformAsset.svg");
      break;
    case plAssetAction::ButtonType::TransformAllAssets:
      SetIconPath(":/EditorFramework/Icons/TransformAllAssets.svg");
      break;
    case plAssetAction::ButtonType::ResaveAllAssets:
      SetIconPath(":/EditorFramework/Icons/ResavAllAssets.svg");
      break;
    case plAssetAction::ButtonType::CheckFileSystem:
      SetIconPath(":/EditorFramework/Icons/CheckFileSystem.svg");
      break;
    case plAssetAction::ButtonType::WriteLookupTable:
      SetIconPath(":/EditorFramework/Icons/WriteLookupTable.svg");
      break;
    case plAssetAction::ButtonType::WriteDependencyDGML:
      break;
  }
}

plAssetAction::~plAssetAction() = default;

void plAssetAction::Execute(const plVariant& value)
{
  switch (m_ButtonType)
  {
    case plAssetAction::ButtonType::TransformAsset:
    {
      if (m_Context.m_pDocument->IsModified())
      {
        plStatus res = const_cast<plDocument*>(m_Context.m_pDocument)->SaveDocument();
        if (res.m_Result.Failed())
        {
          plLog::Error("Failed to save document '{0}': '{1}'", m_Context.m_pDocument->GetDocumentPath(), res.m_sMessage);
          break;
        }
      }

      plTransformStatus ret = plAssetCurator::GetSingleton()->TransformAsset(m_Context.m_pDocument->GetGuid(), plTransformFlags::ForceTransform | plTransformFlags::TriggeredManually);

      if (ret.Failed())
      {
        plLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, m_Context.m_pDocument->GetDocumentPath());
      }
      else
      {
        plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
      }
    }
    break;

    case plAssetAction::ButtonType::TransformAllAssets:
    {
      plAssetCurator::GetSingleton()->CheckFileSystem();
      plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::None).IgnoreResult();
    }
    break;

    case plAssetAction::ButtonType::ResaveAllAssets:
    {
      plAssetCurator::GetSingleton()->ResaveAllAssets();
    }
    break;

    case plAssetAction::ButtonType::CheckFileSystem:
    {
      plAssetCurator::GetSingleton()->CheckFileSystem();
      plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case plAssetAction::ButtonType::WriteLookupTable:
    {
      plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
    }
    break;

    case plAssetAction::ButtonType::WriteDependencyDGML:
    {
      plStringBuilder sOutput = QFileDialog::getSaveFileName(QApplication::activeWindow(), "Write to DGML", {}, "DGML (*.dgml)", nullptr, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

      if (sOutput.IsEmpty())
        return;

      plAssetCurator::GetSingleton()->WriteDependencyDGML(m_Context.m_pDocument->GetGuid(), sOutput);
    }
    break;
  }
}
