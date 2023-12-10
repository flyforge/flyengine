#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Preferences/Preferences.h>

struct PLASMA_EDITORFRAMEWORK_DLL plEngineViewPreferences
{
  plVec3 m_vCamPos = plVec3::MakeZero();
  plVec3 m_vCamDir = plVec3::MakeAxisX();
  plVec3 m_vCamUp = plVec3::MakeAxisZ();
  plSceneViewPerspective::Enum m_PerspectiveMode = plSceneViewPerspective::Perspective;
  plViewRenderMode::Enum m_RenderMode = plViewRenderMode::Default;
  float m_fFov = 70.0f;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_EDITORFRAMEWORK_DLL, plEngineViewPreferences);

class PLASMA_EDITORFRAMEWORK_DLL plQuadViewPreferencesUser : public plPreferences
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plQuadViewPreferencesUser, plPreferences);

public:
  plQuadViewPreferencesUser();

  bool m_bQuadView;
  plEngineViewPreferences m_ViewSingle;
  plEngineViewPreferences m_ViewQuad0;
  plEngineViewPreferences m_ViewQuad1;
  plEngineViewPreferences m_ViewQuad2;
  plEngineViewPreferences m_ViewQuad3;

  plUInt32 FavCams_GetCount() const { return 10; }
  plEngineViewPreferences FavCams_GetCam(plUInt32 i) const { return m_FavoriteCamera[i]; }
  void FavCams_SetCam(plUInt32 i, plEngineViewPreferences cam) { m_FavoriteCamera[i] = cam; }
  void FavCams_Insert(plUInt32 uiIndex, plEngineViewPreferences cam) {}
  void FavCams_Remove(plUInt32 uiIndex) {}

  plEngineViewPreferences m_FavoriteCamera[10];
};
