#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

#include <EditorPluginAmplitudeAudio/Actions/AmplitudeAudioActions.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>

#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>

// BEGIN-DOCS-CODE-SNIPPET: plugin-setup
PL_PLUGIN_ON_LOADED()
{
  plAmplitudeAudioActions::RegisterActions();

  // Control Collection
  {
    // Menu Bar
    plActionMapManager::RegisterActionMap("AudioControlCollectionAssetMenuBar").IgnoreResult();
    plStandardMenus::MapActions("AudioControlCollectionAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
    plProjectActions::MapActions("AudioControlCollectionAssetMenuBar");
    plDocumentActions::MapMenuActions("AudioControlCollectionAssetMenuBar");
    plAssetActions::MapMenuActions("AudioControlCollectionAssetMenuBar");
    plCommandHistoryActions::MapActions("AudioControlCollectionAssetMenuBar");

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("AudioControlCollectionAssetToolBar").IgnoreResult();
      plDocumentActions::MapToolbarActions("AudioControlCollectionAssetToolBar");
      plCommandHistoryActions::MapActions("AudioControlCollectionAssetToolBar", "");
      plAssetActions::MapToolBarActions("AudioControlCollectionAssetToolBar", true);
      plCommonAssetActions::MapToolbarActions("AudioControlCollectionAssetToolBar", plCommonAssetUiState::Grid);
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

PL_PLUGIN_ON_UNLOADED()
{
  plAmplitudeAudioActions::UnregisterActions();
}
// END-DOCS-CODE-SNIPPET
