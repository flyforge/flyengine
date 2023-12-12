#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CameraVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plCameraVisualizerAdapter::plCameraVisualizerAdapter() {}

plCameraVisualizerAdapter::~plCameraVisualizerAdapter() {}

void plCameraVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PLASMA_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  const plCameraVisualizerAttribute* pAttr = static_cast<const plCameraVisualizerAttribute*>(m_pVisualizerAttr);

  m_hBoxGizmo.ConfigureHandle(nullptr, PlasmaEngineGizmoHandleType::LineBox, plColor::DodgerBlue, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);
  m_hFrustumGizmo.ConfigureHandle(nullptr, PlasmaEngineGizmoHandleType::Frustum, plColor::DodgerBlue, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);
  m_hNearPlaneGizmo.ConfigureHandle(nullptr, PlasmaEngineGizmoHandleType::LineRect, plColor::LightBlue, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);
  m_hFarPlaneGizmo.ConfigureHandle(nullptr, PlasmaEngineGizmoHandleType::LineRect, plColor::PaleVioletRed, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hBoxGizmo);
  pAssetDocument->AddSyncObject(&m_hFrustumGizmo);
  pAssetDocument->AddSyncObject(&m_hNearPlaneGizmo);
  pAssetDocument->AddSyncObject(&m_hFarPlaneGizmo);

  m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plCameraVisualizerAdapter::Update()
{

  const plCameraVisualizerAttribute* pAttr = static_cast<const plCameraVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  float fNearPlane = 1.0f;
  float fFarPlane = 10.0f;
  plInt32 iMode = 0;

  if (!pAttr->GetModeProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetModeProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plInt32>(), "Invalid property bound to plCameraVisualizerAttribute 'mode'");
    iMode = value.ConvertTo<plInt32>();
  }

  if (!pAttr->GetNearPlaneProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetNearPlaneProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plCameraVisualizerAttribute 'near plane'");
    fNearPlane = value.ConvertTo<float>();
  }

  if (!pAttr->GetFarPlaneProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFarPlaneProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plCameraVisualizerAttribute 'far plane'");
    fFarPlane = value.ConvertTo<float>();
  }

  if (iMode == plCameraMode::OrthoFixedHeight || iMode == plCameraMode::OrthoFixedWidth)
  {
    float fDimensions = 1.0f;

    if (!pAttr->GetOrthoDimProperty().IsEmpty())
    {
      plVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOrthoDimProperty()), value).IgnoreResult();

      PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plCameraVisualizerAttribute 'ortho dim'");
      fDimensions = value.ConvertTo<float>();
    }

    {
      const float fRange = fFarPlane - fNearPlane;

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fRange, fDimensions, fDimensions);
      m_LocalTransformFrustum.m_vPosition.Set(fNearPlane + fRange * 0.5f, 0, 0);
    }

    m_hBoxGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFrustumGizmo.SetVisible(false);
    m_hNearPlaneGizmo.SetVisible(false);
    m_hFarPlaneGizmo.SetVisible(false);

    m_LocalTransformNearPlane.SetIdentity();
    m_LocalTransformFarPlane.SetIdentity();
  }
  else
  {
    float fFOV = 45.0f;

    if (!pAttr->GetFovProperty().IsEmpty())
    {
      plVariant value;
      pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetFovProperty()), value).IgnoreResult();

      PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plCameraVisualizerAttribute 'fov'");
      fFOV = value.ConvertTo<float>();
    }

    {
      const float fAngleScale = plMath::Tan(plAngle::Degree(fFOV) * 0.5f);
      const float fFrustumScale = plMath::Min(fFarPlane, 10.0f);
      const float fFarPlaneScale = plMath::Min(fFarPlane, 9.0f);
      ;

      // indicate whether the shown far plane is the actual distance, or just the maximum visualization distance
      m_hFarPlaneGizmo.SetColor(fFarPlane > 9.0f ? plColor::DodgerBlue : plColor::PaleVioletRed);

      m_LocalTransformFrustum.m_qRotation.SetIdentity();
      m_LocalTransformFrustum.m_vScale.Set(fFrustumScale, fAngleScale * fFrustumScale, fAngleScale * fFrustumScale);
      m_LocalTransformFrustum.m_vPosition.Set(0, 0, 0);

      m_LocalTransformNearPlane.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
      m_LocalTransformNearPlane.m_vScale.Set(fAngleScale * fNearPlane, fAngleScale * fNearPlane, 1);
      m_LocalTransformNearPlane.m_vPosition.Set(fNearPlane, 0, 0);

      m_LocalTransformFarPlane.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(90));
      m_LocalTransformFarPlane.m_vScale.Set(fAngleScale * fFarPlaneScale, fAngleScale * fFarPlaneScale, 1);
      m_LocalTransformFarPlane.m_vPosition.Set(fFarPlaneScale, 0, 0);
    }

    m_hBoxGizmo.SetVisible(false);
    m_hFrustumGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hNearPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
    m_hFarPlaneGizmo.SetVisible(m_bVisualizerIsVisible);
  }
}

void plCameraVisualizerAdapter::UpdateGizmoTransform()
{
  plTransform t = GetObjectTransform();
  m_hBoxGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hFrustumGizmo.SetTransformation(t * m_LocalTransformFrustum);
  m_hNearPlaneGizmo.SetTransformation(t * m_LocalTransformNearPlane);
  m_hFarPlaneGizmo.SetTransformation(t * m_LocalTransformFarPlane);
}
