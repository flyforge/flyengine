#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plSceneViewPerspective, 1)
  PL_ENUM_CONSTANTS(plSceneViewPerspective::Orthogonal_Front, plSceneViewPerspective::Orthogonal_Right, plSceneViewPerspective::Orthogonal_Top,
    plSceneViewPerspective::Perspective)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEngineViewLightSettings, 1, plRTTIDefaultAllocator<plEngineViewLightSettings>)
  {
    PL_BEGIN_PROPERTIES
    {
      PL_MEMBER_PROPERTY("SkyBox", m_bSkyBox),
      PL_MEMBER_PROPERTY("SkyLight", m_bSkyLight),
      PL_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap),
      PL_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity),
      PL_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight),
      PL_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle),
      PL_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
      PL_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity),
      PL_MEMBER_PROPERTY("Fog", m_bFog)
    }
    PL_END_PROPERTIES;
  }
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plEngineViewConfig::ApplyPerspectiveSetting(float fFov, float fNearPlane, float fFarPlane)
{
  const float fOrthoRange = 1000.0f;

  switch (m_Perspective)
  {
    case plSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovY, fFov == 0.0f ? 70.0f : fFov, fNearPlane, fFarPlane);
    }
    break;

    case plSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(plCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + plVec3(-1, 0, 0), plVec3(0, 0, 1));
    }
    break;

    case plSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(plCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + plVec3(0, -1, 0), plVec3(0, 0, 1));
    }
    break;

    case plSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(plCameraMode::OrthoFixedHeight, fFov == 0.0f ? 20.0f : fFov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + plVec3(0, 0, -1), plVec3(1, 0, 0));
    }
    break;
  }
}

plEngineViewLightSettings::plEngineViewLightSettings(bool bEnable)
{
  if (!bEnable)
  {
    m_bSkyBox = false;
    m_bSkyLight = false;
    m_bDirectionalLight = false;
    m_bFog = false;
  }
}

plEngineViewLightSettings::~plEngineViewLightSettings()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

bool plEngineViewLightSettings::GetSkyBox() const
{
  return m_bSkyBox;
}

void plEngineViewLightSettings::SetSkyBox(bool bVal)
{
  m_bSkyBox = bVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::SkyBoxChanged);
}

bool plEngineViewLightSettings::GetSkyLight() const
{
  return m_bSkyLight;
}

void plEngineViewLightSettings::SetSkyLight(bool bVal)
{
  m_bSkyLight = bVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::SkyLightChanged);
}

const char* plEngineViewLightSettings::GetSkyLightCubeMap() const
{
  return m_sSkyLightCubeMap;
}

void plEngineViewLightSettings::SetSkyLightCubeMap(const char* szVal)
{
  m_sSkyLightCubeMap = szVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
}

float plEngineViewLightSettings::GetSkyLightIntensity() const
{
  return m_fSkyLightIntensity;
}

void plEngineViewLightSettings::SetSkyLightIntensity(float fVal)
{
  m_fSkyLightIntensity = fVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);
}

bool plEngineViewLightSettings::GetDirectionalLight() const
{
  return m_bDirectionalLight;
}

void plEngineViewLightSettings::SetDirectionalLight(bool bVal)
{
  m_bDirectionalLight = bVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
}

plAngle plEngineViewLightSettings::GetDirectionalLightAngle() const
{
  return m_DirectionalLightAngle;
}

void plEngineViewLightSettings::SetDirectionalLightAngle(plAngle val)
{
  m_DirectionalLightAngle = val;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
}

bool plEngineViewLightSettings::GetDirectionalLightShadows() const
{
  return m_bDirectionalLightShadows;
}

void plEngineViewLightSettings::SetDirectionalLightShadows(bool bVal)
{
  m_bDirectionalLightShadows = bVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
}

float plEngineViewLightSettings::GetDirectionalLightIntensity() const
{
  return m_fDirectionalLightIntensity;
}

void plEngineViewLightSettings::SetDirectionalLightIntensity(float fVal)
{
  m_fDirectionalLightIntensity = fVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
}

bool plEngineViewLightSettings::GetFog() const
{
  return m_bFog;
}

void plEngineViewLightSettings::SetFog(bool bVal)
{
  m_bFog = bVal;
  SetModifiedInternal(plEngineViewLightSettingsEvent::Type::FogChanged);
}

bool plEngineViewLightSettings::SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;
  UpdateForEngine(pWorld);
  return false;
}

namespace
{
  template <typename T>
  T* SyncComponent(plWorld* pWorld, plGameObject* pParent, plComponentHandle& inout_hHandle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      T* pComp = nullptr;
      if (inout_hHandle.IsInvalidated() || !pWorld->TryGetComponent(inout_hHandle, pComp))
      {
        inout_hHandle = T::CreateComponent(pParent, pComp);
      }
      return pComp;
    }
    else
    {
      if (!inout_hHandle.IsInvalidated())
      {
        T* pComp = nullptr;
        if (pWorld->TryGetComponent(inout_hHandle, pComp))
        {
          pComp->DeleteComponent();
          inout_hHandle.Invalidate();
        }
      }
      return nullptr;
    }
  }

  plGameObject* SyncGameObject(plWorld* pWorld, plGameObjectHandle& inout_hHandle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      plGameObject* pObj = nullptr;
      if (inout_hHandle.IsInvalidated() || !pWorld->TryGetObject(inout_hHandle, pObj))
      {
        plGameObjectDesc obj;
        obj.m_sName.Assign("ViewLightSettings");
        inout_hHandle = pWorld->CreateObject(obj, pObj);
        pObj->MakeDynamic();
      }
      return pObj;
    }
    else
    {
      if (!inout_hHandle.IsInvalidated())
      {
        pWorld->DeleteObjectDelayed(inout_hHandle);
      }
      return nullptr;
    }
  }
} // namespace

void plEngineViewLightSettings::UpdateForEngine(plWorld* pWorld)
{
  if (plGameObject* pParent = SyncGameObject(m_pWorld, m_hSkyBoxObject, m_bSkyBox))
  {
    pParent->SetTag(plTagRegistry::GetGlobalRegistry().RegisterTag("SkyLight"));

    if (plSkyBoxComponent* pSkyBox = SyncComponent<plSkyBoxComponent>(m_pWorld, pParent, m_hSkyBox, m_bSkyBox))
    {
      pSkyBox->SetCubeMapFile(m_sSkyLightCubeMap);
    }
  }

  const bool bNeedGameObject = m_bDirectionalLight | m_bSkyLight;
  if (plGameObject* pParent = SyncGameObject(m_pWorld, m_hGameObject, bNeedGameObject))
  {
    plQuat rot = plQuat::MakeFromAxisAndAngle(plVec3(0.0f, 1.0f, 0.0f), m_DirectionalLightAngle + plAngle::MakeFromDegree(90.0));
    pParent->SetLocalRotation(rot);

    if (plDirectionalLightComponent* pDirLight = SyncComponent<plDirectionalLightComponent>(m_pWorld, pParent, m_hDirLight, m_bDirectionalLight))
    {
      pDirLight->SetCastShadows(m_bDirectionalLightShadows);
      pDirLight->SetIntensity(m_fDirectionalLightIntensity);
    }

    if (plSkyLightComponent* pSkyLight = SyncComponent<plSkyLightComponent>(m_pWorld, pParent, m_hSkyLight, m_bSkyLight))
    {
      pSkyLight->SetIntensity(m_fSkyLightIntensity);
      pSkyLight->SetReflectionProbeMode(plReflectionProbeMode::Static);
      pSkyLight->SetCubeMapFile(m_sSkyLightCubeMap);
    }

    if (plFogComponent* pFog = SyncComponent<plFogComponent>(m_pWorld, pParent, m_hFog, m_bFog))
    {
      pFog->SetColor(plColor(0.02f, 0.02f, 0.02f));
      pFog->SetDensity(5.0f);
      pFog->SetHeightFalloff(0);
    }
  }
}

void plEngineViewLightSettings::SetModifiedInternal(plEngineViewLightSettingsEvent::Type type)
{
  SetModified();
  plEngineViewLightSettingsEvent e;
  e.m_Type = type;
  m_EngineViewLightSettingsEvents.Broadcast(e);
}
