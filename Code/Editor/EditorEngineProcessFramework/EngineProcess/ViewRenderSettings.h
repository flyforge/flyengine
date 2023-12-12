#pragma once

#include <Core/Graphics/Camera.h>
#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Reflection/Reflection.h>
#include <RendererCore/Pipeline/ViewRenderMode.h>

struct PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plSceneViewPerspective
{
  typedef plUInt8 StorageType;

  enum Enum
  {
    Orthogonal_Front,
    Orthogonal_Right,
    Orthogonal_Top,
    Perspective,

    Default = Perspective
  };
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL, plSceneViewPerspective);

struct PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEngineViewConfig
{
  PlasmaEngineViewConfig()
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
  PlasmaEngineViewConfig* m_pLinkedViewConfig; // used to store which other view config this is linked to, for resetting values when switching views

  void ApplyPerspectiveSetting(float fov = 0.0f, float nearPlane = 0.1f, float farPlane = 1000.0f);
};
struct PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEngineViewLightSettingsEvent
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

class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL PlasmaEngineViewLightSettings : public PlasmaEditorEngineSyncObject
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEngineViewLightSettings, PlasmaEditorEngineSyncObject);

public:
  PlasmaEngineViewLightSettings(bool bEnable = true);
  ~PlasmaEngineViewLightSettings();

  bool GetSkyBox() const;
  void SetSkyBox(bool val);

  bool GetSkyLight() const;
  void SetSkyLight(bool val);

  const char* GetSkyLightCubeMap() const;
  void SetSkyLightCubeMap(const char* val);

  float GetSkyLightIntensity() const;
  void SetSkyLightIntensity(float val);

  bool GetDirectionalLight() const;
  void SetDirectionalLight(bool val);

  plAngle GetDirectionalLightAngle() const;
  void SetDirectionalLightAngle(plAngle val);

  bool GetDirectionalLightShadows() const;
  void SetDirectionalLightShadows(bool val);

  float GetDirectionalLightIntensity() const;
  void SetDirectionalLightIntensity(float val);

  bool GetFog() const;
  void SetFog(bool val);

  mutable plEvent<const PlasmaEngineViewLightSettingsEvent&> m_EngineViewLightSettingsEvents;

  virtual bool SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID) override;
  virtual void UpdateForEngine(plWorld* pWorld) override;

private:
  void SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type type);

  bool m_bSkyBox = true;
  bool m_bSkyLight = true;
  plString m_sSkyLightCubeMap = "{ 0b202e08-a64f-465d-b38e-15b81d161822 }";
  float m_fSkyLightIntensity = 1.0f;

  bool m_bDirectionalLight = true;
  plAngle m_DirectionalLightAngle = plAngle::Degree(30.0f);
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
