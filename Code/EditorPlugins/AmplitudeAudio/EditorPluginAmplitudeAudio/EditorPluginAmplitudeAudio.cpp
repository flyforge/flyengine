#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

#include <EditorPluginAmplitudeAudio/Actions/AmplitudeAudioActions.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

// BEGIN-DOCS-CODE-SNIPPET: plugin-setup
PLASMA_PLUGIN_ON_LOADED()
{
  plAmplitudeAudioActions::RegisterActions();

  // Control Collection
  {
    // Menu Bar
    plActionMapManager::RegisterActionMap("AudioControlCollectionAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("AudioControlCollectionAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
    plProjectActions::MapActions("AudioControlCollectionAssetMenuBar");
    plDocumentActions::MapActions("AudioControlCollectionAssetMenuBar", "Menu.File", false);
    plCommandHistoryActions::MapActions("AudioControlCollectionAssetMenuBar", "Menu.Edit");

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("AudioControlCollectionAssetToolBar").IgnoreResult();
      plDocumentActions::MapActions("AudioControlCollectionAssetToolBar", "", true);
      plCommandHistoryActions::MapActions("AudioControlCollectionAssetToolBar", "");
      plAssetActions::MapToolBarActions("AudioControlCollectionAssetToolBar", true);
      plAmplitudeAudioActions::MapToolbarActions("AudioControlCollectionAssetToolBar");
    }
  }

  // Scene
  {
    // Menu Bar
    {
      plAmplitudeAudioActions::MapMenuActions("EditorPluginScene_DocumentMenuBar");
      plAmplitudeAudioActions::MapMenuActions("EditorPluginScene_Scene2MenuBar");
      plAmplitudeAudioActions::MapToolbarActions("EditorPluginScene_DocumentToolBar");
      plAmplitudeAudioActions::MapToolbarActions("EditorPluginScene_Scene2ToolBar");
    }
  }
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  plAmplitudeAudioActions::UnregisterActions();
}
// END-DOCS-CODE-SNIPPET
