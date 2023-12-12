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
      plActionMapManager::RegisterActionMap("RmlUiAssetMenuBar").IgnoreResult();
      plStandardMenus::MapActions("RmlUiAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
      plProjectActions::MapActions("RmlUiAssetMenuBar");
      plDocumentActions::MapActions("RmlUiAssetMenuBar", "Menu.File", false);
      plCommandHistoryActions::MapActions("RmlUiAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("RmlUiAssetToolBar").IgnoreResult();
      plDocumentActions::MapActions("RmlUiAssetToolBar", "", true);
      plCommandHistoryActions::MapActions("RmlUiAssetToolBar", "");
      plAssetActions::MapToolBarActions("RmlUiAssetToolBar", true);
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
