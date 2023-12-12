#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

void OnLoadPlugin()
{
  // Kraut Tree
  {
    // Menu Bar
    {
      plActionMapManager::RegisterActionMap("KrautTreeAssetMenuBar").IgnoreResult();
      plStandardMenus::MapActions("KrautTreeAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
      plProjectActions::MapActions("KrautTreeAssetMenuBar");
      plDocumentActions::MapActions("KrautTreeAssetMenuBar", "Menu.File", false);
      plAssetActions::MapMenuActions("KrautTreeAssetMenuBar", "Menu.File");
      plCommandHistoryActions::MapActions("KrautTreeAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("KrautTreeAssetToolBar").IgnoreResult();
      plDocumentActions::MapActions("KrautTreeAssetToolBar", "", true);
      plCommandHistoryActions::MapActions("KrautTreeAssetToolBar", "");
      plAssetActions::MapToolBarActions("KrautTreeAssetToolBar", true);
    }
  }
}

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}
