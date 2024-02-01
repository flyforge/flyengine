#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/ConeVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plConeVisualizerAdapter::plConeVisualizerAdapter() = default;

plConeVisualizerAdapter::~plConeVisualizerAdapter() = default;

void plConeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PL_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  const plConeVisualizerAttribute* pAttr = static_cast<const plConeVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, plEngineGizmoHandleType::Cone, pAttr->m_Color, plGizmoFlags::ShowInOrtho | plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plConeVisualizerAdapter::Update()
{
  const plConeVisualizerAttribute* pAttr = static_cast<const plConeVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plAngle>(), "Invalid property bound to plConeVisualizerAttribute 'angle'");
    m_fAngleScale = plMath::Tan(value.ConvertTo<plAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plConeVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<plColor>());
  }

  m_fFinalScale = pAttr->m_fScale;
  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plConeVisualizerAttribute 'radius'");
    m_fFinalScale *= value.ConvertTo<float>();
  }

  m_hGizmo.SetVisible(m_bVisualizerIsVisible && m_fAngleScale != 0.0f && m_fFinalScale != 0.0f);
}

void plConeVisualizerAdapter::UpdateGizmoTransform()
{
  const plConeVisualizerAttribute* pAttr = static_cast<const plConeVisualizerAttribute*>(m_pVisualizerAttr);

  const plQuat axisRotation = plBasisAxis::GetBasisRotation_PosX(pAttr->m_Axis);

  plTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(plVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fFinalScale);
  t.m_qRotation = axisRotation * t.m_qRotation;
  m_hGizmo.SetTransformation(t);
}
