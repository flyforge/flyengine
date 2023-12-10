#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CapsuleVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plCapsuleVisualizerAdapter::plCapsuleVisualizerAdapter() = default;
plCapsuleVisualizerAdapter::~plCapsuleVisualizerAdapter() = default;

void plCapsuleVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PLASMA_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");
  PLASMA_MSVC_ANALYSIS_ASSUME(pAssetDocument != nullptr);

  const plCapsuleVisualizerAttribute* pAttr = static_cast<const plCapsuleVisualizerAttribute*>(m_pVisualizerAttr);

  m_hCylinder.ConfigureHandle(nullptr, plEngineGizmoHandleType::CylinderZ, pAttr->m_Color, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);
  m_hSphereTop.ConfigureHandle(nullptr, plEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);
  m_hSphereBottom.ConfigureHandle(nullptr, plEngineGizmoHandleType::HalfSphereZ, pAttr->m_Color, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hCylinder);
  pAssetDocument->AddSyncObject(&m_hSphereTop);
  pAssetDocument->AddSyncObject(&m_hSphereBottom);

  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);
}

void plCapsuleVisualizerAdapter::Update()
{
  const plCapsuleVisualizerAttribute* pAttr = static_cast<const plCapsuleVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
  m_hSphereTop.SetVisible(m_bVisualizerIsVisible);
  m_hSphereBottom.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;
  m_Anchor = pAttr->m_Anchor;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    PLASMA_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to plCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    plVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value).AssertSuccess();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to plCapsuleVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value).AssertSuccess();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plCapsuleVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plCapsuleVisualizerAttribute 'color'");
    m_hSphereTop.SetColor(value.ConvertTo<plColor>() * pAttr->m_Color);
    m_hSphereBottom.SetColor(value.ConvertTo<plColor>() * pAttr->m_Color);
    m_hCylinder.SetColor(value.ConvertTo<plColor>() * pAttr->m_Color);
  }
}

void plCapsuleVisualizerAdapter::UpdateGizmoTransform()
{
  plVec3 vOffset = plVec3::MakeZero();

  if (m_Anchor.IsSet(plVisualizerAnchor::PosX))
    vOffset.x -= m_fRadius;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegX))
    vOffset.x += m_fRadius;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosY))
    vOffset.y -= m_fRadius;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegY))
    vOffset.y += m_fRadius;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosZ))
    vOffset.z -= m_fRadius + 0.5f * m_fHeight;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegZ))
    vOffset.z += m_fRadius + 0.5f * m_fHeight;

  plTransform tSphereTop;
  tSphereTop.SetIdentity();
  tSphereTop.m_vScale = plVec3(m_fRadius);
  tSphereTop.m_vPosition.z = m_fHeight * 0.5f;
  tSphereTop.m_vPosition += vOffset;

  plTransform tSphereBottom;
  tSphereBottom.SetIdentity();
  tSphereBottom.m_vScale = plVec3(m_fRadius, -m_fRadius, -m_fRadius);
  tSphereBottom.m_vPosition.z = -m_fHeight * 0.5f;
  tSphereBottom.m_vPosition += vOffset;

  plTransform tCylinder;
  tCylinder.SetIdentity();
  tCylinder.m_vScale = plVec3(m_fRadius, m_fRadius, m_fHeight);
  tCylinder.m_vPosition += vOffset;

  m_hSphereTop.SetTransformation(GetObjectTransform() * tSphereTop);
  m_hSphereBottom.SetTransformation(GetObjectTransform() * tSphereBottom);
  m_hCylinder.SetTransformation(GetObjectTransform() * tCylinder);
}
