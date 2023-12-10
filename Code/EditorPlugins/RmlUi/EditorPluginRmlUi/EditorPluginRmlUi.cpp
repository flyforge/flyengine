#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin()
{
  // RmlUi
  {
    // Menu Bar
    {
      // Menu Bar
      {
        plActionMapManager::RegisterActionMap("RmlUiAssetMenuBar").IgnoreResult();
        plStandardMenus::MapActions("RmlUiAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
        plProjectActions::MapActions("RmlUiAssetMenuBar");
        plDocumentActions::MapMenuActions("RmlUiAssetMenuBar");
        plAssetActions::MapMenuActions("RmlUiAssetMenuBar");
        plCommandHistoryActions::MapActions("RmlUiAssetMenuBar");
      }

      // Tool Bar
      {
        plActionMapManager::RegisterActionMap("RmlUiAssetToolBar").IgnoreResult();
        plDocumentActions::MapToolbarActions("RmlUiAssetToolBar");
        plCommandHistoryActions::MapActions("RmlUiAssetToolBar", "");
        plAssetActions::MapToolBarActions("RmlUiAssetToolBar", true);
      }
    }
  }
}

void OnUnloadPlugin() {}

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
