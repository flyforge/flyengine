#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Configuration/SubSystem.h>

plAngle plSnapProvider::s_RotationSnapValue = plAngle::MakeFromDegree(15.0f);
float plSnapProvider::s_fScaleSnapValue = 0.125f;
float plSnapProvider::s_fTranslationSnapValue = 0.25f;
plEventSubscriptionID plSnapProvider::s_UserPreferencesChanged = 0;

plEvent<const plSnapProviderEvent&> plSnapProvider::s_Events;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorFramework, SnapProvider)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "EditorFrameworkMain"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plSnapProvider::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plSnapProvider::Shutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

void plSnapProvider::Startup()
{
  plQtEditorApp::m_Events.AddEventHandler(plMakeDelegate(&plSnapProvider::EditorEventHandler));
}

void plSnapProvider::Shutdown()
{
  if (s_UserPreferencesChanged)
  {
    plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
    pPreferences->m_ChangedEvent.RemoveEventHandler(s_UserPreferencesChanged);
  }
  plQtEditorApp::m_Events.RemoveEventHandler(plMakeDelegate(&plSnapProvider::EditorEventHandler));
}

void plSnapProvider::EditorEventHandler(const plEditorAppEvent& e)
{
  if (e.m_Type == plEditorAppEvent::Type::EditorStarted)
  {
    plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
    PreferenceChangedEventHandler(pPreferences);
    s_UserPreferencesChanged = pPreferences->m_ChangedEvent.AddEventHandler(plMakeDelegate(&plSnapProvider::PreferenceChangedEventHandler));
  }
}

void plSnapProvider::PreferenceChangedEventHandler(plPreferences* pPreferenceBase)
{
  auto* pPreferences = static_cast<plEditorPreferencesUser*>(pPreferenceBase);
  SetRotationSnapValue(pPreferences->m_RotationSnapValue);
  SetScaleSnapValue(pPreferences->m_fScaleSnapValue);
  SetTranslationSnapValue(pPreferences->m_fTranslationSnapValue);
}

plAngle plSnapProvider::GetRotationSnapValue()
{
  return s_RotationSnapValue;
}

float plSnapProvider::GetScaleSnapValue()
{
  return s_fScaleSnapValue;
}

float plSnapProvider::GetTranslationSnapValue()
{
  return s_fTranslationSnapValue;
}

void plSnapProvider::SetRotationSnapValue(plAngle angle)
{
  if (s_RotationSnapValue == angle)
    return;

  s_RotationSnapValue = angle;

  plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
  pPreferences->m_RotationSnapValue = angle;
  pPreferences->TriggerPreferencesChangedEvent();

  plSnapProviderEvent e;
  e.m_Type = plSnapProviderEvent::Type::RotationSnapChanged;
  s_Events.Broadcast(e);
}

void plSnapProvider::SetScaleSnapValue(float fPercentage)
{
  if (s_fScaleSnapValue == fPercentage)
    return;

  s_fScaleSnapValue = fPercentage;

  plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
  pPreferences->m_fScaleSnapValue = fPercentage;
  pPreferences->TriggerPreferencesChangedEvent();

  plSnapProviderEvent e;
  e.m_Type = plSnapProviderEvent::Type::ScaleSnapChanged;
  s_Events.Broadcast(e);
}

void plSnapProvider::SetTranslationSnapValue(float fUnits)
{
  if (s_fTranslationSnapValue == fUnits)
    return;

  s_fTranslationSnapValue = fUnits;

  plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
  pPreferences->m_fTranslationSnapValue = fUnits;
  pPreferences->TriggerPreferencesChangedEvent();

  plSnapProviderEvent e;
  e.m_Type = plSnapProviderEvent::Type::TranslationSnapChanged;
  s_Events.Broadcast(e);
}

void plSnapProvider::SnapTranslation(plVec3& value)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  value.x = plMath::RoundToMultiple(value.x, s_fTranslationSnapValue);
  value.y = plMath::RoundToMultiple(value.y, s_fTranslationSnapValue);
  value.z = plMath::RoundToMultiple(value.z, s_fTranslationSnapValue);
}

void plSnapProvider::SnapTranslationInLocalSpace(const plQuat& qRotation, plVec3& ref_vTranslation)
{
  if (s_fTranslationSnapValue <= 0.0f)
    return;

  const plQuat mInvRot = qRotation.GetInverse();

  plVec3 vLocalTranslation = mInvRot * ref_vTranslation;
  vLocalTranslation.x = plMath::RoundToMultiple(vLocalTranslation.x, s_fTranslationSnapValue);
  vLocalTranslation.y = plMath::RoundToMultiple(vLocalTranslation.y, s_fTranslationSnapValue);
  vLocalTranslation.z = plMath::RoundToMultiple(vLocalTranslation.z, s_fTranslationSnapValue);

  ref_vTranslation = qRotation * vLocalTranslation;
}

void plSnapProvider::SnapRotation(plAngle& ref_rotation)
{
  if (s_RotationSnapValue.GetRadian() != 0.0f)
  {
    ref_rotation = plAngle::MakeFromRadian(plMath::RoundToMultiple(ref_rotation.GetRadian(), s_RotationSnapValue.GetRadian()));
  }
}

void plSnapProvider::SnapScale(float& ref_fScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    ref_fScale = plMath::RoundToMultiple(ref_fScale, s_fScaleSnapValue);
  }
}

void plSnapProvider::SnapScale(plVec3& ref_vScale)
{
  if (s_fScaleSnapValue > 0.0f)
  {
    SnapScale(ref_vScale.x);
    SnapScale(ref_vScale.y);
    SnapScale(ref_vScale.z);
  }
}

plVec3 plSnapProvider::GetScaleSnapped(const plVec3& vScale)
{
  plVec3 res = vScale;
  SnapScale(res);
  return res;
}
