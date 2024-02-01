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
      plDocumentActions::MapMenuActions("VisualScriptAssetMenuBar");
      plAssetActions::MapMenuActions("VisualScriptAssetMenuBar");
      plCommandHistoryActions::MapActions("VisualScriptAssetMenuBar");
      plEditActions::MapActions("VisualScriptAssetMenuBar", false, false);
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("VisualScriptAssetToolBar").IgnoreResult();
      plDocumentActions::MapToolbarActions("VisualScriptAssetToolBar");
      plCommandHistoryActions::MapActions("VisualScriptAssetToolBar", "");
      plAssetActions::MapToolBarActions("VisualScriptAssetToolBar", true);
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
