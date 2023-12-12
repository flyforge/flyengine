#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plSceneViewPerspective, 1)
  PLASMA_ENUM_CONSTANTS(plSceneViewPerspective::Orthogonal_Front, plSceneViewPerspective::Orthogonal_Right, plSceneViewPerspective::Orthogonal_Top,
    plSceneViewPerspective::Perspective)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(PlasmaEngineViewLightSettings, 1, plRTTIDefaultAllocator<PlasmaEngineViewLightSettings>)
  {
    PLASMA_BEGIN_PROPERTIES
    {
      PLASMA_MEMBER_PROPERTY("SkyBox", m_bSkyBox),
      PLASMA_MEMBER_PROPERTY("SkyLight", m_bSkyLight),
      PLASMA_MEMBER_PROPERTY("SkyLightCubeMap", m_sSkyLightCubeMap),
      PLASMA_MEMBER_PROPERTY("SkyLightIntensity", m_fSkyLightIntensity),
      PLASMA_MEMBER_PROPERTY("DirectionalLight", m_bDirectionalLight),
      PLASMA_MEMBER_PROPERTY("DirectionalLightAngle", m_DirectionalLightAngle),
      PLASMA_MEMBER_PROPERTY("DirectionalLightShadows", m_bDirectionalLightShadows),
      PLASMA_MEMBER_PROPERTY("DirectionalLightIntensity", m_fDirectionalLightIntensity),
      PLASMA_MEMBER_PROPERTY("Fog", m_bFog)
    }
    PLASMA_END_PROPERTIES;
  }
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void PlasmaEngineViewConfig::ApplyPerspectiveSetting(float fov, float nearPlane, float farPlane)
{
  const float fOrthoRange = 1000.0f;

  switch (m_Perspective)
  {
    case plSceneViewPerspective::Perspective:
    {
      m_Camera.SetCameraMode(plCameraMode::PerspectiveFixedFovY, fov == 0.0f ? 70.0f : fov, nearPlane, farPlane);
    }
    break;

    case plSceneViewPerspective::Orthogonal_Front:
    {
      m_Camera.SetCameraMode(plCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + plVec3(-1, 0, 0), plVec3(0, 0, 1));
    }
    break;

    case plSceneViewPerspective::Orthogonal_Right:
    {
      m_Camera.SetCameraMode(plCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + plVec3(0, -1, 0), plVec3(0, 0, 1));
    }
    break;

    case plSceneViewPerspective::Orthogonal_Top:
    {
      m_Camera.SetCameraMode(plCameraMode::OrthoFixedHeight, fov == 0.0f ? 20.0f : fov, -fOrthoRange, fOrthoRange);
      m_Camera.LookAt(m_Camera.GetCenterPosition(), m_Camera.GetCenterPosition() + plVec3(0, 0, -1), plVec3(1, 0, 0));
    }
    break;
  }
}

PlasmaEngineViewLightSettings::PlasmaEngineViewLightSettings(bool bEnable)
{
  if (!bEnable)
  {
    m_bSkyBox = false;
    m_bSkyLight = false;
    m_bDirectionalLight = false;
    m_bFog = false;
  }
}

PlasmaEngineViewLightSettings::~PlasmaEngineViewLightSettings()
{
  if (m_hGameObject.IsInvalidated())
    return;

  m_pWorld->DeleteObjectDelayed(m_hGameObject);
}

bool PlasmaEngineViewLightSettings::GetSkyBox() const
{
  return m_bSkyBox;
}

void PlasmaEngineViewLightSettings::SetSkyBox(bool val)
{
  m_bSkyBox = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::SkyBoxChanged);
}

bool PlasmaEngineViewLightSettings::GetSkyLight() const
{
  return m_bSkyLight;
}

void PlasmaEngineViewLightSettings::SetSkyLight(bool val)
{
  m_bSkyLight = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::SkyLightChanged);
}

const char* PlasmaEngineViewLightSettings::GetSkyLightCubeMap() const
{
  return m_sSkyLightCubeMap;
}

void PlasmaEngineViewLightSettings::SetSkyLightCubeMap(const char* val)
{
  m_sSkyLightCubeMap = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::SkyLightCubeMapChanged);
}

float PlasmaEngineViewLightSettings::GetSkyLightIntensity() const
{
  return m_fSkyLightIntensity;
}

void PlasmaEngineViewLightSettings::SetSkyLightIntensity(float val)
{
  m_fSkyLightIntensity = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::SkyLightIntensityChanged);
}

bool PlasmaEngineViewLightSettings::GetDirectionalLight() const
{
  return m_bDirectionalLight;
}

void PlasmaEngineViewLightSettings::SetDirectionalLight(bool val)
{
  m_bDirectionalLight = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::DirectionalLightChanged);
}

plAngle PlasmaEngineViewLightSettings::GetDirectionalLightAngle() const
{
  return m_DirectionalLightAngle;
}

void PlasmaEngineViewLightSettings::SetDirectionalLightAngle(plAngle val)
{
  m_DirectionalLightAngle = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::DirectionalLightAngleChanged);
}

bool PlasmaEngineViewLightSettings::GetDirectionalLightShadows() const
{
  return m_bDirectionalLightShadows;
}

void PlasmaEngineViewLightSettings::SetDirectionalLightShadows(bool val)
{
  m_bDirectionalLightShadows = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::DirectionalLightShadowsChanged);
}

float PlasmaEngineViewLightSettings::GetDirectionalLightIntensity() const
{
  return m_fDirectionalLightIntensity;
}

void PlasmaEngineViewLightSettings::SetDirectionalLightIntensity(float val)
{
  m_fDirectionalLightIntensity = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::DirectionalLightIntensityChanged);
}

bool PlasmaEngineViewLightSettings::GetFog() const
{
  return m_bFog;
}

void PlasmaEngineViewLightSettings::SetFog(bool val)
{
  m_bFog = val;
  SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type::FogChanged);
}

bool PlasmaEngineViewLightSettings::SetupForEngine(plWorld* pWorld, plUInt32 uiNextComponentPickingID)
{
  m_pWorld = pWorld;
  UpdateForEngine(pWorld);
  return false;
}

namespace
{
  template <typename T>
  T* SyncComponent(plWorld* pWorld, plGameObject* pParent, plComponentHandle& handle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      T* pComp = nullptr;
      if (handle.IsInvalidated() || !pWorld->TryGetComponent(handle, pComp))
      {
        handle = T::CreateComponent(pParent, pComp);
      }
      return pComp;
    }
    else
    {
      if (!handle.IsInvalidated())
      {
        T* pComp = nullptr;
        if (pWorld->TryGetComponent(handle, pComp))
        {
          pComp->DeleteComponent();
          handle.Invalidate();
        }
      }
      return nullptr;
    }
  }

  plGameObject* SyncGameObject(plWorld* pWorld, plGameObjectHandle& handle, bool bShouldExist)
  {
    if (bShouldExist)
    {
      plGameObject* pObj = nullptr;
      if (handle.IsInvalidated() || !pWorld->TryGetObject(handle, pObj))
      {
        plGameObjectDesc obj;
        obj.m_sName.Assign("ViewLightSettings");
        handle = pWorld->CreateObject(obj, pObj);
        pObj->MakeDynamic();
      }
      return pObj;
    }
    else
    {
      if (!handle.IsInvalidated())
      {
        pWorld->DeleteObjectDelayed(handle);
      }
      return nullptr;
    }
  }
} // namespace

void PlasmaEngineViewLightSettings::UpdateForEngine(plWorld* pWorld)
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
    plQuat rot;
    rot.SetFromAxisAndAngle(plVec3(0.0f, 1.0f, 0.0f), m_DirectionalLightAngle + plAngle::Degree(90.0));
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

void PlasmaEngineViewLightSettings::SetModifiedInternal(PlasmaEngineViewLightSettingsEvent::Type type)
{
  SetModified();
  PlasmaEngineViewLightSettingsEvent e;
  e.m_Type = type;
  m_EngineViewLightSettingsEvents.Broadcast(e);
}
