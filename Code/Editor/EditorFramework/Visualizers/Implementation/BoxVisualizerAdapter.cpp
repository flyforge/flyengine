#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/BoxVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plBoxVisualizerAdapter::plBoxVisualizerAdapter() = default;
plBoxVisualizerAdapter::~plBoxVisualizerAdapter() = default;

void plBoxVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PL_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  const plBoxVisualizerAttribute* pAttr = static_cast<const plBoxVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, plEngineGizmoHandleType::LineBox, pAttr->m_Color, plGizmoFlags::Visualizer | plGizmoFlags::ShowInOrtho);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plBoxVisualizerAdapter::Update()
{
  const plBoxVisualizerAttribute* pAttr = static_cast<const plBoxVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(pAttr->m_fSizeScale);

  if (!pAttr->GetSizeProperty().IsEmpty())
  {
    m_vScale *= pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetSizeProperty()));
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();
    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plBoxVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<plColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plVec3>(), "Invalid property bound to plBoxVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<plVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<plVec3>());
  }

  m_qRotation.SetIdentity();

  if (!pAttr->GetRotationProperty().IsEmpty())
  {
    m_qRotation = pObjectAccessor->Get<plQuat>(m_pObject, GetProperty(pAttr->GetRotationProperty()));
  }

  m_Anchor = pAttr->m_Anchor;
}

void plBoxVisualizerAdapter::UpdateGizmoTransform()
{
  plTransform t;
  t.m_vScale = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  plVec3 vOffset = plVec3::MakeZero();

  if (m_Anchor.IsSet(plVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x * 0.5f;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y * 0.5f;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z * 0.5f;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z * 0.5f;

  t.m_vPosition += vOffset;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
