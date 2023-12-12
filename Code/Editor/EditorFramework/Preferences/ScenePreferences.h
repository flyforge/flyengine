#pragma once

#include <EditorFramework/Preferences/Preferences.h>

class PLASMA_EDITORFRAMEWORK_DLL plScenePreferencesUser : public plPreferences
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plScenePreferencesUser, plPreferences);

public:
  plScenePreferencesUser();

  void SetCameraSpeed(plInt32 value);
  plInt32 GetCameraSpeed() const { return m_iCameraSpeed; }

  void SetShowGrid(bool show);
  bool GetShowGrid() const { return m_bShowGrid; }

protected:
  bool m_bShowGrid;
  int m_iCameraSpeed;
};
