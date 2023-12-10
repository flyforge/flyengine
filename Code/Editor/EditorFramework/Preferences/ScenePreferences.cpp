#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Preferences/ScenePreferences.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plScenePreferencesUser, 1, plRTTIDefaultAllocator<plScenePreferencesUser>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ShowGrid", m_bShowGrid),
    PLASMA_MEMBER_PROPERTY("CameraSpeed", m_iCameraSpeed)->AddAttributes(new plDefaultValueAttribute(10), new plClampValueAttribute(1, 30)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScenePreferencesUser::plScenePreferencesUser()
  : plPreferences(Domain::Document, "Scene")
{
  m_iCameraSpeed = 9;
}

void plScenePreferencesUser::SetCameraSpeed(plInt32 value)
{
  m_iCameraSpeed = plMath::Clamp(value, 0, 24);

  // Kiff, inform the men!
  TriggerPreferencesChangedEvent();
}

void plScenePreferencesUser::SetShowGrid(bool bShow)
{
  m_bShowGrid = bShow;

  TriggerPreferencesChangedEvent();
}
