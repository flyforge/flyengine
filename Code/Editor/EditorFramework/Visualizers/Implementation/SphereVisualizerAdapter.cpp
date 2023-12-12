#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/SphereVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plSphereVisualizerAdapter::plSphereVisualizerAdapter() = default;
plSphereVisualizerAdapter::~plSphereVisualizerAdapter() = default;

void plSphereVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PLASMA_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  const plSphereVisualizerAttribute* pAttr = static_cast<const plSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, PlasmaEngineGizmoHandleType::Sphere, pAttr->m_Color, plGizmoFlags::ShowInOrtho | plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plSphereVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plSphereVisualizerAttribute* pAttr = static_cast<const plSphereVisualizerAttribute*>(m_pVisualizerAttr);

  m_fScale = 1.0f;

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRadiusProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plSphereVisualizerAttribute 'radius'");
    m_fScale = value.ConvertTo<float>();
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plSphereVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<plColor>() * pAttr->m_Color);
  }

  m_vPositionOffset = pAttr->m_vOffsetOrScale;

  if (!pAttr->GetOffsetProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetOffsetProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plVec3>(), "Invalid property bound to plSphereVisualizerAttribute 'offset'");

    if (m_vPositionOffset.IsZero())
      m_vPositionOffset = value.ConvertTo<plVec3>();
    else
      m_vPositionOffset = m_vPositionOffset.CompMul(value.ConvertTo<plVec3>());
  }

  m_Anchor = pAttr->m_Anchor;
}

void plSphereVisualizerAdapter::UpdateGizmoTransform()
{
  plTransform t;
  t.m_qRotation.SetIdentity();
  t.m_vScale.Set(m_fScale);
  t.m_vPosition = m_vPositionOffset;

  plVec3 vOffset = plVec3::ZeroVector();

  if (m_Anchor.IsSet(plVisualizerAnchor::PosX))
    vOffset.x -= t.m_vScale.x;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegX))
    vOffset.x += t.m_vScale.x;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosY))
    vOffset.y -= t.m_vScale.y;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegY))
    vOffset.y += t.m_vScale.y;
  if (m_Anchor.IsSet(plVisualizerAnchor::PosZ))
    vOffset.z -= t.m_vScale.z;
  if (m_Anchor.IsSet(plVisualizerAnchor::NegZ))
    vOffset.z += t.m_vScale.z;

  t.m_vPosition += vOffset;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
