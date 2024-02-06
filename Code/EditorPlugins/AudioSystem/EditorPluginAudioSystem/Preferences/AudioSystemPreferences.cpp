#include <EditorPluginAudioSystem/EditorPluginAudioSystemPCH.h>

#include <EditorPluginAudioSystem/Preferences/AudioSystemPreferences.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioSystemProjectPreferences, 1, plRTTIDefaultAllocator<plAudioSystemProjectPreferences>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Mute", m_bMute),
    PL_MEMBER_PROPERTY("Gain", m_fGain)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 1.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAudioSystemProjectPreferences::plAudioSystemProjectPreferences()
  : plPreferences(Domain::Project, "AudioSystem")
{
  plEditorEngineProcessConnection::s_Events.AddEventHandler(plMakeDelegate(&plAudioSystemProjectPreferences::ProcessEventHandler, this));
}

plAudioSystemProjectPreferences::~plAudioSystemProjectPreferences()
{
  plEditorEngineProcessConnection::s_Events.RemoveEventHandler(plMakeDelegate(&plAudioSystemProjectPreferences::ProcessEventHandler, this));
}

void plAudioSystemProjectPreferences::SetMute(bool bMute)
{
  m_bMute = bMute;
  SyncCVars();
}

void plAudioSystemProjectPreferences::SetGain(float fGain)
{
  m_fGain = fGain;
  SyncCVars();
}

void plAudioSystemProjectPreferences::SyncCVars()
{
  TriggerPreferencesChangedEvent();

  {
    plChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "Audio.MasterGain";
    msg.m_NewValue = m_fGain;

    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  {
    plChangeCVarMsgToEngine msg;
    msg.m_sCVarName = "Audio.Mute";
    msg.m_NewValue = m_bMute;

    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
}

void plAudioSystemProjectPreferences::ProcessEventHandler(const plEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == plEditorEngineProcessConnection::Event::Type::ProcessRestarted)
  {
    SyncCVars();
  }
}
