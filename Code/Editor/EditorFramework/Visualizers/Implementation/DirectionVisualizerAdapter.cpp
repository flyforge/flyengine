#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Visualizers/DirectionVisualizerAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plDirectionVisualizerAdapter::plDirectionVisualizerAdapter() {}

plDirectionVisualizerAdapter::~plDirectionVisualizerAdapter() {}

void plDirectionVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PLASMA_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  const plDirectionVisualizerAttribute* pAttr = static_cast<const plDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  m_hGizmo.ConfigureHandle(nullptr, PlasmaEngineGizmoHandleType::Arrow, pAttr->m_Color, plGizmoFlags::ShowInOrtho | plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plDirectionVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  const plDirectionVisualizerAttribute* pAttr = static_cast<const plDirectionVisualizerAttribute*>(m_pVisualizerAttr);

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plDirectionVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<plColor>() * pAttr->m_Color);
  }
}

void plDirectionVisualizerAdapter::UpdateGizmoTransform()
{
  const plDirectionVisualizerAttribute* pAttr = static_cast<const plDirectionVisualizerAttribute*>(m_pVisualizerAttr);
  float fScale = pAttr->m_fScale;

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetLengthProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<float>(), "Invalid property bound to plDirectionVisualizerAttribute 'length'");
    fScale *= value.ConvertTo<float>();
  }

  plBasisAxis::Enum axis = pAttr->m_Axis;

  if (!pAttr->GetAxisProperty().IsEmpty())
  {
    plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();

    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAxisProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plInt32>(), "Invalid property bound to plDirectionVisualizerAttribute 'length'");

    axis = static_cast<plBasisAxis::Enum>(value.ConvertTo<plInt32>());
  }

  const plQuat axisRotation = plBasisAxis::GetBasisRotation_PosX(axis);

  plTransform t;
  t.m_qRotation = axisRotation;
  t.m_vScale = plVec3(fScale);
  t.m_vPosition = axisRotation * plVec3(fScale * 0.5f, 0, 0);

  plTransform tObject = GetObjectTransform();
  tObject.m_vScale.Set(1.0f);

  m_hGizmo.SetTransformation(tObject * t);
}
