#include <EditorPluginAudioSystem/EditorPluginAudioSystemPCH.h>

#include <EditorPluginAudioSystem/Actions/AudioSystemActions.h>
//#include <EditorPluginAudioSystem/Dialogs/AudioSystemProjectSettingsDlg.moc.h>
#include <EditorPluginAudioSystem/Preferences/AudioSystemPreferences.h>

#include <GuiFoundation/Action/ActionManager.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemSliderAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plAudioSystemActions::s_hCategoryAudioSystem;
plActionDescriptorHandle plAudioSystemActions::s_hMuteSound;
plActionDescriptorHandle plAudioSystemActions::s_hMasterVolume;

void plAudioSystemActions::RegisterActions()
{
  s_hCategoryAudioSystem = PL_REGISTER_CATEGORY("AudioSystem");

  s_hMuteSound = PL_REGISTER_ACTION_1(
    "AudioSystem.Mute",
    plActionScope::Document,
    "AudioSystem",
    "",
    plAudioSystemAction,
    plAudioSystemAction::ActionType::MuteSound
  );

  s_hMasterVolume = PL_REGISTER_ACTION_1(
    "AudioSystem.MasterVolume",
    plActionScope::Document,
    "Volume",
    "",
    plAudioSystemSliderAction,
    plAudioSystemSliderAction::ActionType::MasterVolume
  );
}

void plAudioSystemActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategoryAudioSystem);
  plActionManager::UnregisterAction(s_hMuteSound);
  plActionManager::UnregisterAction(s_hMasterVolume);
}

void plAudioSystemActions::MapMenuActions(const char* szMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PL_ASSERT_DEV(pMap != nullptr, "Mapping the actions failed!");

  pMap->MapAction(s_hCategoryAudioSystem, "G.Plugins.Settings", 5.0f);
  pMap->MapAction(s_hMuteSound, "G.Plugins.Settings", 0.0f);
  pMap->MapAction(s_hMasterVolume, "G.Plugins.Settings", 2.0f);
}

void plAudioSystemActions::MapToolbarActions(const char* szMapping)
{
  plActionMap* pSceneMap = plActionMapManager::GetActionMap(szMapping);
  PL_ASSERT_DEV(pSceneMap != nullptr, "Mapping the actions failed!");

  pSceneMap->MapAction(s_hCategoryAudioSystem, "", 12.0f);
  pSceneMap->MapAction(s_hMuteSound, "AudioSystem", 0.0f);
}

plAudioSystemAction::plAudioSystemAction(const plActionContext& context, const char* szName, ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::MuteSound:
    {
      SetCheckable(true);

      const auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();
      pPreferences->m_ChangedEvent.AddEventHandler(plMakeDelegate(&plAudioSystemAction::OnPreferenceChange, this));

      if (pPreferences->GetMute())
        SetIconPath(":/Icons/SoundOff.svg");
      else
        SetIconPath(":/Icons/SoundOn.svg");

      SetChecked(pPreferences->GetMute());
    }
    break;
  }
}

plAudioSystemAction::~plAudioSystemAction()
{
  if (m_Type == ActionType::MuteSound)
  {
    const auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(plMakeDelegate(&plAudioSystemAction::OnPreferenceChange, this));
  }
}

void plAudioSystemAction::Execute(const plVariant& value)
{
  if (m_Type == ActionType::MuteSound)
  {
    auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();
    pPreferences->SetMute(!pPreferences->GetMute());

    if (GetContext().m_pDocument)
    {
      GetContext().m_pDocument->ShowDocumentStatus(plFmt("Sound is {}", pPreferences->GetMute() ? "muted" : "on"));
    }
  }
}

void plAudioSystemAction::OnPreferenceChange(plPreferences* pref)
{
  const auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();

  if (m_Type == ActionType::MuteSound)
  {
    if (pPreferences->GetMute())
      SetIconPath(":/Icons/SoundOff.svg");
    else
      SetIconPath(":/Icons/SoundOn.svg");

    SetChecked(pPreferences->GetMute());
  }
}

//////////////////////////////////////////////////////////////////////////

plAudioSystemSliderAction::plAudioSystemSliderAction(const plActionContext& context, const char* szName, ActionType type)
  : plSliderAction(context, szName)
{
  m_Type = type;

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      const auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();
      pPreferences->m_ChangedEvent.AddEventHandler(plMakeDelegate(&plAudioSystemSliderAction::OnPreferenceChange, this));
      SetRange(0, 100);
    }
    break;
  }

  UpdateState();
}

plAudioSystemSliderAction::~plAudioSystemSliderAction()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      const auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();
      pPreferences->m_ChangedEvent.RemoveEventHandler(plMakeDelegate(&plAudioSystemSliderAction::OnPreferenceChange, this));
    }
    break;
  }
}

void plAudioSystemSliderAction::Execute(const plVariant& value)
{
  const plInt32 iValue = value.Get<plInt32>();

  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();

      pPreferences->SetGain(iValue / 100.0f);

      if (GetContext().m_pDocument)
      {
        GetContext().m_pDocument->ShowDocumentStatus(plFmt("Sound Volume: {}%%", static_cast<plInt32>(pPreferences->GetGain() * 100.0f)));
      }
    }
    break;
  }
}

void plAudioSystemSliderAction::OnPreferenceChange(plPreferences* pref)
{
  UpdateState();
}

void plAudioSystemSliderAction::UpdateState()
{
  switch (m_Type)
  {
    case ActionType::MasterVolume:
    {
      const auto* pPreferences = plPreferences::QueryPreferences<plAudioSystemProjectPreferences>();
      SetValue(plMath::Clamp(static_cast<plInt32>(pPreferences->GetGain() * 100.0f), 0, 100));
    }
    break;
  }
}
