#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEditorPreferencesUser, 1, plRTTIDefaultAllocator<plEditorPreferencesUser>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("ShowSplashscreen", m_bShowSplashscreen)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("BackgroundAssetProcessing", m_bBackgroundAssetProcessing)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new plDefaultValueAttribute(70.0f), new plClampValueAttribute(10.0f, 150.0f)),
    PL_MEMBER_PROPERTY("MaxFramerate", m_uiMaxFramerate)->AddAttributes(new plDefaultValueAttribute(60)),
    PL_ACCESSOR_PROPERTY("GizmoSize", GetGizmoSize, SetGizmoSize)->AddAttributes(new plDefaultValueAttribute(1.5f), new plClampValueAttribute(0.2f, 5.0f)),
    PL_ACCESSOR_PROPERTY("ShowInDevelopmentFeatures", GetShowInDevelopmentFeatures, SetShowInDevelopmentFeatures),
    PL_MEMBER_PROPERTY("RotationSnap", m_RotationSnapValue)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(15.0f)), new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("ScaleSnap", m_fScaleSnapValue)->AddAttributes(new plDefaultValueAttribute(0.125f), new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("TranslationSnap", m_fTranslationSnapValue)->AddAttributes(new plDefaultValueAttribute(0.25f), new plHiddenAttribute()),
    PL_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("CustomPrecompiledToolsFolder", m_sCustomPrecompiledToolsFolder),
    PL_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("ClearEditorLogsOnPlay", m_bClearEditorLogsOnPlay)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ACCESSOR_PROPERTY("HighlightUntranslatedUI", GetHighlightUntranslatedUI, SetHighlightUntranslatedUI),

    // START GROUP Engine View Light Settings
    PL_MEMBER_PROPERTY("SkyBox", m_bSkyBox)->AddAttributes(new plDefaultValueAttribute(true), new plGroupAttribute("Engine View Light Settings")),
    PL_MEMBER_PROPERTY("SkyLight", m_bSkyLight)->AddAttributes(new plDefaultValueAttribute(true), new plClampValueAttribute(0.0f, 2.0f)),
    PL_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap)->AddAttributes(new plDefaultValueAttribute(plStringView("{ 0b202e08-a64f-465d-b38e-15b81d161822 }")), new plAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    PL_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 20.0f)),
    PL_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(30.0f)), new plClampValueAttribute(plAngle::MakeFromDegree(-90.0f), plAngle::MakeFromDegree(90.0f))),
    PL_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
    PL_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)->AddAttributes(new plDefaultValueAttribute(10.0f)),
    PL_MEMBER_PROPERTY("Fog", m_bFog),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plEditorPreferencesUser::plEditorPreferencesUser()
  : plPreferences(Domain::Application, "General")
{
}

plEditorPreferencesUser::~plEditorPreferencesUser() = default;

void plEditorPreferencesUser::ApplyDefaultValues(plEngineViewLightSettings& ref_settings)
{
  ref_settings.SetSkyBox(m_bSkyBox);
  ref_settings.SetSkyLight(m_bSkyLight);
  ref_settings.SetSkyLightCubeMap(m_sSkyLightCubeMap);
  ref_settings.SetSkyLightIntensity(m_fSkyLightIntensity);
  ref_settings.SetDirectionalLight(m_bDirectionalLight);
  ref_settings.SetDirectionalLightAngle(m_DirectionalLightAngle);
  ref_settings.SetDirectionalLightShadows(m_bDirectionalLightShadows);
  ref_settings.SetDirectionalLightIntensity(m_fDirectionalLightIntensity);
  ref_settings.SetFog(m_bFog);
}

void plEditorPreferencesUser::SetAsDefaultValues(const plEngineViewLightSettings& settings)
{
  m_bSkyBox = settings.GetSkyBox();
  m_bSkyLight = settings.GetSkyLight();
  m_sSkyLightCubeMap = settings.GetSkyLightCubeMap();
  m_fSkyLightIntensity = settings.GetSkyLightIntensity();
  m_bDirectionalLight = settings.GetDirectionalLight();
  m_DirectionalLightAngle = settings.GetDirectionalLightAngle();
  m_bDirectionalLightShadows = settings.GetDirectionalLightShadows();
  m_fDirectionalLightIntensity = settings.GetDirectionalLightIntensity();
  m_bFog = settings.GetFog();
  TriggerPreferencesChangedEvent();
}

void plEditorPreferencesUser::SetShowInDevelopmentFeatures(bool b)
{
  m_bShowInDevelopmentFeatures = b;

  plQtAddSubElementButton::s_bShowInDevelopmentFeatures = b;
}

void plEditorPreferencesUser::SetHighlightUntranslatedUI(bool b)
{
  m_bHighlightUntranslatedUI = b;

  plTranslator::HighlightUntranslated(m_bHighlightUntranslatedUI);
}

void plEditorPreferencesUser::SetGizmoSize(float f)
{
  m_fGizmoSize = f;
  SyncGlobalSettings();
}


void plEditorPreferencesUser::SetMaxFramerate(plUInt16 uiFPS)
{
  if (m_uiMaxFramerate == uiFPS)
    return;

  m_uiMaxFramerate = uiFPS;
}

void plEditorPreferencesUser::SyncGlobalSettings()
{
  plGlobalSettingsMsgToEngine msg;
  msg.m_fGizmoScale = m_fGizmoSize;

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtEditorApp::LoadEditorPreferences()
{
  PL_PROFILE_SCOPE("Preferences");
  plPreferences::QueryPreferences<plEditorPreferencesUser>();
}
