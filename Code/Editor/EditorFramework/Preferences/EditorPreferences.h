#pragma once

#include <EditorFramework/Preferences/Preferences.h>
#include <Foundation/Strings/String.h>

class plEngineViewLightSettings;

/// \brief Stores editor specific preferences for the current user
class PL_EDITORFRAMEWORK_DLL plEditorPreferencesUser : public plPreferences
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorPreferencesUser, plPreferences);

public:
  plEditorPreferencesUser();
  ~plEditorPreferencesUser();

  void ApplyDefaultValues(plEngineViewLightSettings& ref_settings);
  void SetAsDefaultValues(const plEngineViewLightSettings& settings);

  float m_fPerspectiveFieldOfView = 70.0f;
  plAngle m_RotationSnapValue = plAngle::MakeFromDegree(15.0f);
  float m_fScaleSnapValue = 0.125f;
  float m_fTranslationSnapValue = 0.25f;
  bool m_bUsePrecompiledTools = true;
  plString m_sCustomPrecompiledToolsFolder;
  bool m_bLoadLastProjectAtStartup = true;
  bool m_bShowSplashscreen = true;
  bool m_bExpandSceneTreeOnSelection = true;
  bool m_bBackgroundAssetProcessing = true;
  bool m_bHighlightUntranslatedUI = false;

  bool m_bSkyBox = true;
  bool m_bSkyLight = true;
  plString m_sSkyLightCubeMap = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float m_fSkyLightIntensity = 1.0f;
  bool m_bDirectionalLight = true;
  plAngle m_DirectionalLightAngle = plAngle::MakeFromDegree(30.0f);
  bool m_bDirectionalLightShadows = false;
  float m_fDirectionalLightIntensity = 10.0f;
  bool m_bFog = false;
  bool m_bClearEditorLogsOnPlay = true;

  void SetShowInDevelopmentFeatures(bool b);
  bool GetShowInDevelopmentFeatures() const
  {
    return m_bShowInDevelopmentFeatures;
  }

  void SetHighlightUntranslatedUI(bool b);
  bool GetHighlightUntranslatedUI() const
  {
    return m_bHighlightUntranslatedUI;
  }

  void SetGizmoSize(float f);
  float GetGizmoSize() const { return m_fGizmoSize; }

  void SetMaxFramerate(plUInt16 uiFPS);
  plUInt16 GetMaxFramerate() const { return m_uiMaxFramerate; }

private:
  void SyncGlobalSettings();

  float m_fGizmoSize = 1.5f;
  bool m_bShowInDevelopmentFeatures = false;
  plUInt16 m_uiMaxFramerate = 60;
};
