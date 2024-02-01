#include <EditorPluginSubstance/EditorPluginSubstancePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

static void ToolsProjectEventHandler(const plToolsProjectEvent& e);

void OnLoadPlugin()
{
  // Asset
  {
    // Menu Bar
    {
      const char* szMenuBar = "SubstanceAssetMenuBar";

  plActionMapManager::RegisterActionMap(szMenuBar).IgnoreResult();
  plStandardMenus::MapActions(szMenuBar, plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
  plProjectActions::MapActions(szMenuBar);
  plDocumentActions::MapMenuActions(szMenuBar);
  plAssetActions::MapMenuActions(szMenuBar);
  plCommandHistoryActions::MapActions(szMenuBar);

  plEditActions::MapActions("SubstanceAssetMenuBar", false, false);
}

// Tool Bar
{
  const char* szToolBar = "SubstanceAssetToolBar";
  plActionMapManager::RegisterActionMap(szToolBar).IgnoreResult();
  plDocumentActions::MapToolbarActions(szToolBar);
  plCommandHistoryActions::MapActions(szToolBar, "");
  plAssetActions::MapToolBarActions(szToolBar, true);
}
}

// Scene
{
  // Menu Bar
  {}

  // Tool Bar
  {
  }
}
}

void OnUnloadPlugin()
{
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
