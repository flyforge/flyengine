#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const plToolsProjectEvent& e);

void OnLoadPlugin()
{
  plToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh
  {
    plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      plActionMapManager::RegisterActionMap("JoltCollisionMeshAssetMenuBar").IgnoreResult();
      plStandardMenus::MapActions("JoltCollisionMeshAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
      plProjectActions::MapActions("JoltCollisionMeshAssetMenuBar");
      plDocumentActions::MapActions("JoltCollisionMeshAssetMenuBar", "Menu.File", false);
      plAssetActions::MapMenuActions("JoltCollisionMeshAssetMenuBar", "Menu.File");
      plCommandHistoryActions::MapActions("JoltCollisionMeshAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("JoltCollisionMeshAssetToolBar").IgnoreResult();
      plDocumentActions::MapActions("JoltCollisionMeshAssetToolBar", "", true);
      plCommandHistoryActions::MapActions("JoltCollisionMeshAssetToolBar", "");
      plAssetActions::MapToolBarActions("JoltCollisionMeshAssetToolBar", true);
      plCommonAssetActions::MapActions("JoltCollisionMeshAssetToolBar", "", plCommonAssetUiState::Grid);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      plJoltActions::RegisterActions();
      plJoltActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  plJoltActions::UnregisterActions();
  plToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);
}

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateCollisionLayerDynamicEnumValues()
{
  auto& cfe = plDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  cfe.Clear();

  plCollisionFilterConfig cfg;
  if (cfg.Load().Failed())
  {
    return;
  }

  // add all names and values that are valid (non-empty)
  for (plInt32 i = 0; i < 32; ++i)
  {
    if (!plStringUtils::IsNullOrEmpty(cfg.GetGroupName(i)))
    {
      cfe.SetValueAndName(i, cfg.GetGroupName(i));
    }
  }
}

static void ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  if (e.m_Type == plToolsProjectEvent::Type::ProjectSaveState)
  {
    plQtJoltProjectSettingsDlg::EnsureConfigFileExists();
  }

  if (e.m_Type == plToolsProjectEvent::Type::ProjectOpened)
  {
    UpdateCollisionLayerDynamicEnumValues();
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plJoltRopeComponentPatch_1_2 : public plGraphPatch
{
public:
  plJoltRopeComponentPatch_1_2()
    : plGraphPatch("plJoltRopeComponent", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

plJoltRopeComponentPatch_1_2 g_plJoltRopeComponentPatch_1_2;