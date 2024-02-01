#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/CylinderVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plCylinderVisualizerAdapter::plCylinderVisualizerAdapter() = default;

plCylinderVisualizerAdapter::~plCylinderVisualizerAdapter() = default;

void plCylinderVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PL_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  const plCylinderVisualizerAttribute* pAttr = static_cast<const plCylinderVisualizerAttribute*>(m_pVisualizerAttr);

  m_hCylinder.ConfigureHandle(nullptr, plEngineGizmoHandleType::CylinderZ, pAttr->m_Color, plGizmoFlags::ShowInOrtho | plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hCylinder);

  m_hCylinder.SetVisible(m_bVisualizerIsVisible);
}

void plCylinderVisualizerAdapter::Update()
{
  const plCylinderVisualizerAttribute* pAttr = static_cast<const plCylinderVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hCylinder.SetVisible(m_bVisualizerIsVisible);

  m_fRadius = 1.0f;
  m_fHeight = 0.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    auto pProp = GetProperty(pAttr->GetRadiusProperty());
    PL_ASSERT_DEBUG(pProp != nullptr, "Invalid property '{0}' bound to plCylinderVisualizerAttribute 'radius'", pAttr->GetRadiusProperty());

    if (pProp == nullptr)
      return;

    plVariant value;
    pObjectAccessor->GetValue(m_pObject, pProp, value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property '{0}' bound to plCylinderVisualizerAttribute 'radius'",
      pAttr->GetRadiusProperty());
    m_fRadius = value.ConvertTo<float>();
  }

  if (!pAttr->GetHeightProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetHeightProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plCylinderVisualizerAttribute 'height'");
    m_fHeight = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plCylinderVisualizerAttribute 'color'");
    m_hCylinder.SetColor(value.ConvertTo<plColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plVec3>(), "Invalid property bound to plCylinderVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<plVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<plVec3>());
  }

  if (!pAttr->GetAxisProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAxisProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plInt32>(), "Invalid property bound to plCylinderVisualizerAttribute 'axis'");

    m_Axis = static_cast<plBasisAxis::Enum>(value.ConvertTo<plInt32>());
  }
  else
  {
    m_Axis = pAttr->m_Axis;
  }

  m_Anchor = pAttr->m_Anchor;
}

void plCylinderVisualizerAdapter::UpdateGizmoTransform()
{
  const plQuat axisRotation = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveZ, m_Axis);

  plTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale = plVec3(m_fRadius, m_fRadius, m_fHeight);
  t.m_vPosition = m_vPositionOffset;

  plVec3 vOffset = plVec3::MakeZero();

  if (m_Anchor.IsSet(plVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  // plTransform doesn't (can't) combine rotations with scales
  // however, here we know that the axisRotation is just an axis remapping, so we can combine them
  plTransform parentTransform = GetObjectTransform();
  plTransform newTrans = parentTransform * t;
  newTrans.m_vScale = (axisRotation * parentTransform.m_vScale).CompMul(t.m_vScale);

  m_hCylinder.SetTransformation(newTrans);
}
