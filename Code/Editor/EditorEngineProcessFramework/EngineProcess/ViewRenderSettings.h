#pragma once

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>

struct PL_EDITORENGINEPROCESSFRAMEWORK_DLL plSceneViewPerspective
{
  using StorageType = plUInt8;

  enum Enum
  {
    Orthogonal_Front,
    Orthogonal_Right,
    Orthogonal_Top,
    Perspective,

    Default = Perspective
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORENGINEPROCESSFRAMEWORK_DLL, plSceneViewPerspective);

struct PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEngineViewConfig
{
  plEngineViewConfig()
  {
    m_RenderMode = plViewRenderMode::Default;
    m_Perspective = plSceneViewPerspective::Default;
    m_CameraUsageHint = plCameraUsageHint::EditorView;
    m_pLinkedViewConfig = nullptr;
  }

  plViewRenderMode::Enum m_RenderMode;
  plSceneViewPerspective::Enum m_Perspective;
  plCameraUsageHint::Enum m_CameraUsageHint;
  bool m_bUseCameraTransformOnDevice = true;

  plCamera m_Camera;
  plEngineViewConfig* m_pLinkedViewConfig; // used to store which other view config this is linked to, for resetting values when switching views

  void ApplyPerspectiveSetting(float fFov = 0.0f, float fNearPlane = 0.1f, float fFarPlane = 1000.0f);
};
struct PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEngineViewLightSettingsEvent
{
  enum class Type
  {
    SkyBoxChanged,
    SkyLightChanged,
    SkyLightCubeMapChanged,
    SkyLightIntensityChanged,
    DirectionalLightChanged,
    DirectionalLightAngleChanged,
    DirectionalLightShadowsChanged,
    DirectionalLightIntensityChanged,
    FogChanged,
    DefaultValuesChanged,
  };

  Type m_Type;
};

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEngineViewLightSettings : public plEditorEngineSyncObject
{
  PL_ADD_DYNAMIC_REFLECTION(plEngineViewLightSettings, plEditorEngineSyncObject);

public:
  plEngineViewLightSettings(bool bEnable = true);
  ~plEngineViewLightSettings();

  bool GetSkyBox() const;
  void SetSkyBox(bool bVal);

  bool GetSkyLight() const;
  void SetSkyLight(bool bVal);

  const char* GetSkyLightCubeMap() const;
  void SetSkyLightCubeMap(const char* szVal);

  float GetSkyLightIntensity() const;
  void SetSkyLightIntensity(float fVal);

  bool GetDirectionalLight() const;
  void SetDirectionalLight(bool bVal);

  plAngle GetDirectionalLightAngle() const;
  void SetDirectionalLightAngle(plAngle val);

  bool GetDirectionalLightShadows() const;
  void SetDirectionalLightShadows(bool bVal);

  float GetDirectionalLightIntensity() const;
  void SetDirectionalLightIntensity(float fVal);

  bool GetFog() const;
  void SetFog(bool bVal);

  mutable plEvent<const plEngineViewLightSettingsEvent&> m_EngineViewLightSettingsEvents;

  virtual bool SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(plWorld* pWorld) override;

private:
  void SetModifiedInternal(plEngineViewLightSettingsEvent::Type type);

  bool m_bSkyBox = false;
  bool m_bSkyLight = true;
  plString m_sSkyLightCubeMap = "{ 6449d7e0-a8ff-4b43-9f84-df1c870a4748 }";
  float m_fSkyLightIntensity = 1.0f;

  bool m_bDirectionalLight = true;
  plAngle m_DirectionalLightAngle = plAngle::MakeFromDegree(30.0f);
  bool m_bDirectionalLightShadows = false;
  float m_fDirectionalLightIntensity = 10.0f;

  bool m_bFog = false;

  // Engine side data
  plWorld* m_pWorld = nullptr;
  plGameObjectHandle m_hSkyBoxObject;
  plComponentHandle m_hSkyBox;
  plGameObjectHandle m_hGameObject;
  plComponentHandle m_hDirLight;
  plComponentHandle m_hSkyLight;
  plComponentHandle m_hFog;
};
