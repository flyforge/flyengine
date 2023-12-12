#include <EditorPluginVisualScript/EditorPluginVisualScriptPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

void OnLoadPlugin()
{
  // VisualScript
  {
    // Menu Bar
    {
      plActionMapManager::RegisterActionMap("VisualScriptAssetMenuBar").IgnoreResult();
      plStandardMenus::MapActions("VisualScriptAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
      plProjectActions::MapActions("VisualScriptAssetMenuBar");
      plDocumentActions::MapActions("VisualScriptAssetMenuBar", "Menu.File", false);
      plAssetActions::MapMenuActions("VisualScriptAssetMenuBar", "Menu.File");
      plCommandHistoryActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit");
      plEditActions::MapActions("VisualScriptAssetMenuBar", "Menu.Edit", false, false);
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("VisualScriptAssetToolBar").IgnoreResult();
      plDocumentActions::MapActions("VisualScriptAssetToolBar", "", true);
      plCommandHistoryActions::MapActions("VisualScriptAssetToolBar", "");
      plAssetActions::MapToolBarActions("VisualScriptAssetToolBar", true);
    }
  }
}

void OnUnloadPlugin()
{
}

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
