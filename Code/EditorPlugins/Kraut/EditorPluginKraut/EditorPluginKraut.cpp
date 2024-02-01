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
      plStandardMenus::MapActions("KrautTreeAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
      plProjectActions::MapActions("KrautTreeAssetMenuBar");
      plDocumentActions::MapMenuActions("KrautTreeAssetMenuBar");
      plAssetActions::MapMenuActions("KrautTreeAssetMenuBar");
      plCommandHistoryActions::MapActions("KrautTreeAssetMenuBar");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("KrautTreeAssetToolBar").IgnoreResult();
      plDocumentActions::MapToolbarActions("KrautTreeAssetToolBar");
      plCommandHistoryActions::MapActions("KrautTreeAssetToolBar", "");
      plAssetActions::MapToolBarActions("KrautTreeAssetToolBar", true);
    }
  }
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}
