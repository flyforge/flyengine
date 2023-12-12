#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Actions/ViewLightActions.h>
#include <EditorPluginParticle/Actions/ParticleActions.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

void OnLoadPlugin()
{
  plParticleActions::RegisterActions();

  // Particle Effect
  {
    // Menu Bar
    {
      plActionMapManager::RegisterActionMap("ParticleEffectAssetMenuBar").IgnoreResult();
      plStandardMenus::MapActions("ParticleEffectAssetMenuBar", plStandardMenuTypes::File | plStandardMenuTypes::Edit | plStandardMenuTypes::Panels | plStandardMenuTypes::Help);
      plProjectActions::MapActions("ParticleEffectAssetMenuBar");
      plDocumentActions::MapActions("ParticleEffectAssetMenuBar", "Menu.File", false);
      plCommandHistoryActions::MapActions("ParticleEffectAssetMenuBar", "Menu.Edit");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("ParticleEffectAssetToolBar").IgnoreResult();
      plDocumentActions::MapActions("ParticleEffectAssetToolBar", "", true);
      plCommandHistoryActions::MapActions("ParticleEffectAssetToolBar", "");
      plAssetActions::MapToolBarActions("ParticleEffectAssetToolBar", true);
      plParticleActions::MapActions("ParticleEffectAssetToolBar", "");
    }

    // View Tool Bar
    {
      plActionMapManager::RegisterActionMap("ParticleEffectAssetViewToolBar").IgnoreResult();
      plViewActions::MapActions("ParticleEffectAssetViewToolBar", "", plViewActions::RenderMode | plViewActions::ActivateRemoteProcess);
      plViewLightActions::MapActions("ParticleEffectAssetViewToolBar", "");
    }

    plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plParticleEffectAssetDocument::PropertyMetaStateEventHandler);
  }
}

void OnUnloadPlugin()
{
  plParticleActions::UnregisterActions();
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plParticleEffectAssetDocument::PropertyMetaStateEventHandler);
}

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
