#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Assets/AssetCurator.h>

plActionDescriptorHandle plAssetActions::s_hAssetCategory;
plActionDescriptorHandle plAssetActions::s_hTransformAsset;
plActionDescriptorHandle plAssetActions::s_hTransformAllAssets;
plActionDescriptorHandle plAssetActions::s_hResaveAllAssets;
plActionDescriptorHandle plAssetActions::s_hCheckFileSystem;
plActionDescriptorHandle plAssetActions::s_hWriteLookupTable;


void plAssetActions::RegisterActions()
{
  s_hAssetCategory = PLASMA_REGISTER_CATEGORY("AssetCategory");
  s_hTransformAsset = PLASMA_REGISTER_ACTION_1("Asset.Transform", plActionScope::Document, "Assets", "Ctrl+E", plAssetAction, plAssetAction::ButtonType::TransformAsset);
  s_hTransformAllAssets = PLASMA_REGISTER_ACTION_1("Asset.TransformAll", plActionScope::Global, "Assets", "Ctrl+Shift+E", plAssetAction, plAssetAction::ButtonType::TransformAllAssets);
  s_hResaveAllAssets = PLASMA_REGISTER_ACTION_1("Asset.ResaveAll", plActionScope::Global, "Assets", "", plAssetAction, plAssetAction::ButtonType::ResaveAllAssets);
  s_hCheckFileSystem = PLASMA_REGISTER_ACTION_1("Asset.CheckFilesystem", plActionScope::Global, "Assets", "", plAssetAction, plAssetAction::ButtonType::CheckFileSystem);
  s_hWriteLookupTable = PLASMA_REGISTER_ACTION_1("Asset.WriteLookupTable", plActionScope::Global, "Assets", "", plAssetAction, plAssetAction::ButtonType::WriteLookupTable);
}

void plAssetActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hAssetCategory);
  plActionManager::UnregisterAction(s_hTransformAsset);
  plActionManager::UnregisterAction(s_hTransformAllAssets);
  plActionManager::UnregisterAction(s_hResaveAllAssets);
  plActionManager::UnregisterAction(s_hCheckFileSystem);
  plActionManager::UnregisterAction(s_hWriteLookupTable);
}

void plAssetActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the documents actions failed!", szMapping);

  pMap->MapAction(s_hAssetCategory, szPath, 1.5f);
  plStringBuilder sSubPath(szPath, "/AssetCategory");

  pMap->MapAction(s_hTransformAsset, sSubPath, 1.0f);
}

void plAssetActions::MapToolBarActions(const char* szMapping, bool bDocument)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hAssetCategory, "", 10.0f);

  if (bDocument)
  {
    pMap->MapAction(s_hTransformAsset, "AssetCategory", 1.0f);
  }
  else
  {
    pMap->MapAction(s_hCheckFileSystem, "AssetCategory", 0.0f);
    pMap->MapAction(s_hTransformAllAssets, "AssetCategory", 3.0f);
    pMap->MapAction(s_hResaveAllAssets, "AssetCategory", 4.0f);
    // pMap->MapAction(s_hWriteLookupTable, "AssetCategory", 5.0f);
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
  }
}

plAssetAction::~plAssetAction() {}

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
      plAssetCurator::GetSingleton()->TransformAllAssets(plTransformFlags::None).LogFailure();
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
  }
}
