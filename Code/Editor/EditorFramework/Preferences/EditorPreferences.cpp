#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/PropertyGrid/Implementation/AddSubElementButton.moc.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEditorPreferencesUser, 1, plRTTIDefaultAllocator<PlasmaEditorPreferencesUser>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RestoreProjectOnStartup", m_bLoadLastProjectAtStartup)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("ShowSplashscreen", m_bShowSplashscreen)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("BackgroundAssetProcessing", m_bBackgroundAssetProcessing)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("FieldOfView", m_fPerspectiveFieldOfView)->AddAttributes(new plDefaultValueAttribute(70.0f), new plClampValueAttribute(10.0f, 150.0f)),
    PLASMA_ACCESSOR_PROPERTY("GizmoSize", GetGizmoSize, SetGizmoSize)->AddAttributes(new plDefaultValueAttribute(1.5f), new plClampValueAttribute(0.2f, 5.0f)),
    PLASMA_MEMBER_PROPERTY("UseOldGizmos", m_bOldGizmos),
    PLASMA_ACCESSOR_PROPERTY("ShowInDevelopmentFeatures", GetShowInDevelopmentFeatures, SetShowInDevelopmentFeatures),
    PLASMA_MEMBER_PROPERTY("RotationSnap", m_RotationSnapValue)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(15.0f))),
    PLASMA_MEMBER_PROPERTY("ScaleSnap", m_fScaleSnapValue)->AddAttributes(new plDefaultValueAttribute(0.125f)),
    PLASMA_MEMBER_PROPERTY("TranslationSnap", m_fTranslationSnapValue)->AddAttributes(new plDefaultValueAttribute(0.25f)),
    PLASMA_MEMBER_PROPERTY("UsePrecompiledTools", m_bUsePrecompiledTools)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("ExpandSceneTreeOnSelection", m_bExpandSceneTreeOnSelection)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("AssetFilterCombobox", m_bAssetFilterCombobox)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("ClearEditorLogsOnPlay", m_bClearEditorLogsOnPlay)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ACCESSOR_PROPERTY("HighlightUntranslatedUI", GetHighlightUntranslatedUI, SetHighlightUntranslatedUI),

    // START GROUP Engine View Light Settings
    PLASMA_MEMBER_PROPERTY("SkyBox", m_bSkyBox)->AddAttributes(new plDefaultValueAttribute(true), new plGroupAttribute("Engine View Light Settings")),
    PLASMA_MEMBER_PROPERTY("SkyLight", m_bSkyLight)->AddAttributes(new plDefaultValueAttribute(true), new plClampValueAttribute(0.0f, 2.0f)),
    PLASMA_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap)->AddAttributes(new plDefaultValueAttribute(plStringView("{ 0b202e08-a64f-465d-b38e-15b81d161822 }")), new plAssetBrowserAttribute("CompatibleAsset_Texture_Cube")),
    PLASMA_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 20.0f)),
    PLASMA_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(30.0f)), new plClampValueAttribute(plAngle::Degree(-90.0f), plAngle::Degree(90.0f))),
    PLASMA_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
    PLASMA_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity)->AddAttributes(new plDefaultValueAttribute(10.0f)),
    PLASMA_MEMBER_PROPERTY("Fog", m_bFog),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PlasmaEditorPreferencesUser::PlasmaEditorPreferencesUser()
  : plPreferences(Domain::Application, "General")
{
}

PlasmaEditorPreferencesUser::~PlasmaEditorPreferencesUser() = default;

void PlasmaEditorPreferencesUser::ApplyDefaultValues(PlasmaEngineViewLightSettings& settings)
{
  settings.SetSkyBox(m_bSkyBox);
  settings.SetSkyLight(m_bSkyLight);
  settings.SetSkyLightCubeMap(m_sSkyLightCubeMap);
  settings.SetSkyLightIntensity(m_fSkyLightIntensity);
  settings.SetDirectionalLight(m_bDirectionalLight);
  settings.SetDirectionalLightAngle(m_DirectionalLightAngle);
  settings.SetDirectionalLightShadows(m_bDirectionalLightShadows);
  settings.SetDirectionalLightIntensity(m_fDirectionalLightIntensity);
  settings.SetFog(m_bFog);
}

void PlasmaEditorPreferencesUser::SetAsDefaultValues(const PlasmaEngineViewLightSettings& settings)
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

void PlasmaEditorPreferencesUser::SetShowInDevelopmentFeatures(bool b)
{
  m_bShowInDevelopmentFeatures = b;

  plQtAddSubElementButton::s_bShowInDevelopmentFeatures = b;
}

void PlasmaEditorPreferencesUser::SetHighlightUntranslatedUI(bool b)
{
  m_bHighlightUntranslatedUI = b;

  plTranslator::HighlightUntranslated(m_bHighlightUntranslatedUI);
}

void PlasmaEditorPreferencesUser::SetGizmoSize(float f)
{
  m_fGizmoSize = f;
  SyncGlobalSettings();
}

void PlasmaEditorPreferencesUser::SyncGlobalSettings()
{
  plGlobalSettingsMsgToEngine msg;
  msg.m_fGizmoScale = m_fGizmoSize;

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtEditorApp::LoadEditorPreferences()
{
  PLASMA_PROFILE_SCOPE("Preferences");
  plPreferences::QueryPreferences<PlasmaEditorPreferencesUser>();
}
