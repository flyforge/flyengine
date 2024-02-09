#include <EditorPluginAudioSystem/EditorPluginAudioSystemPCH.h>

#include <EditorPluginAudioSystem/Actions/AudioSystemActions.h>
#include <EditorPluginAudioSystem/Preferences/AudioSystemPreferences.h>

#include <EditorFramework/Actions/ProjectActions.h>

static void ToolsProjectEventHandler(const plToolsProjectEvent& e);

void OnLoadPlugin()
{
  plToolsProject::s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Scene
  {
    // Menu Bar
    {
      plAudioSystemActions::RegisterActions();
      plAudioSystemActions::MapMenuActions("EditorPluginScene_DocumentMenuBar");
      plAudioSystemActions::MapMenuActions("EditorPluginScene_Scene2MenuBar");
      plAudioSystemActions::MapToolbarActions("EditorPluginScene_DocumentToolBar");
      plAudioSystemActions::MapToolbarActions("EditorPluginScene_Scene2ToolBar");
    }
  }
}

void OnUnloadPlugin()
{
  plAudioSystemActions::UnregisterActions();
  plToolsProject::s_Events.RemoveEventHandler(ToolsProjectEventHandler);
}

static void ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  if (e.m_Type == plToolsProjectEvent::Type::ProjectOpened)
  {
    plAudioSystemProjectPreferences* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();
    pPreferences->SyncCVars();
  }
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
