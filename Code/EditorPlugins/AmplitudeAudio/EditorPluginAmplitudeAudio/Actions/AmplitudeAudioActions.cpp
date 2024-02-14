#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

#include <EditorPluginAmplitudeAudio/Actions/AmplitudeAudioActions.h>
#include <EditorPluginAmplitudeAudio/Core/AmplitudeAudioControlsManager.h>
#include <EditorPluginAmplitudeAudio/Dialogs/AmplitudeAudioSettingsDialog.moc.h>

#include <GuiFoundation/Action/ActionManager.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAmplitudeAudioAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plAmplitudeAudioActions::s_hCategoryAudioSystem;
plActionDescriptorHandle plAmplitudeAudioActions::s_hProjectSettings;
plActionDescriptorHandle plAmplitudeAudioActions::s_hReloadControls;

void plAmplitudeAudioActions::RegisterActions()
{
  s_hCategoryAudioSystem = PL_REGISTER_CATEGORY("Amplitude");

  s_hProjectSettings = PL_REGISTER_ACTION_1(
    "AmplitudeAudio.Settings.Project",
    plActionScope::Document,
    "Amplitude",
    "",
    plAmplitudeAudioAction,
    plAmplitudeAudioAction::ActionType::ProjectSettings);

  s_hReloadControls = PL_REGISTER_ACTION_1(
    "AmplitudeAudio.ReloadControls",
    plActionScope::Document,
    "Amplitude",
    "",
    plAmplitudeAudioAction,
    plAmplitudeAudioAction::ActionType::ReloadControls);
}

void plAmplitudeAudioActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategoryAudioSystem);
  plActionManager::UnregisterAction(s_hProjectSettings);
  plActionManager::UnregisterAction(s_hReloadControls);
}

void plAmplitudeAudioActions::MapMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryAudioSystem, "G.Plugins.Settings", 10.0f);
  pMap->MapAction(s_hProjectSettings, "G.Plugins.Settings", "Amplitude", 1.0f);

  pMap->MapAction(s_hCategoryAudioSystem, "G.Scene", 0.9f);
  pMap->MapAction(s_hReloadControls, "Amplitude", 1.0f);
}

void plAmplitudeAudioActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pSceneMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pSceneMap != nullptr, "Mapping the actions failed!");

  pSceneMap->MapAction(s_hCategoryAudioSystem, "", 12.0f);
  pSceneMap->MapAction(s_hReloadControls, "Amplitude", 0.0f);
}

plAmplitudeAudioAction::plAmplitudeAudioAction(const plActionContext& context, const char* szName, ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::ProjectSettings:
      SetIconPath(":/AssetIcons/Audio_Control_Collection.svg");
      break;

    case ActionType::ReloadControls:
      SetIconPath(":/AssetIcons/Audio_Control_Collection.svg");
      SetCheckable(false);
      break;
  }
}

plAmplitudeAudioAction::~plAmplitudeAudioAction()
{
}

void plAmplitudeAudioAction::Execute(const plVariant& value)
{
  if (m_Type == ActionType::ProjectSettings)
  {
    plQtAmplitudeAudioSettingsDialog dlg(nullptr);
    dlg.exec();
  }

  if (m_Type == ActionType::ReloadControls)
  {
    auto* pControlsManager = plAmplitudeAudioControlsManager::GetSingleton();
    plLog::Info("Reload Amplitude Audio Controls {}", pControlsManager->ReloadControls().Succeeded() ? "successful" : "failed");
  }
}
