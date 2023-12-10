#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/BoxReflectionProbeVisualizerAdapter.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plBoxReflectionProbeVisualizerAdapter::plBoxReflectionProbeVisualizerAdapter() {}
plBoxReflectionProbeVisualizerAdapter::~plBoxReflectionProbeVisualizerAdapter() {}

void plBoxReflectionProbeVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PLASMA_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, plEngineGizmoHandleType::LineBox, plColorScheme::LightUI(plColorScheme::Yellow), plGizmoFlags::ShowInOrtho | plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plBoxReflectionProbeVisualizerAdapter::Update()
{
  const plBoxReflectionProbeVisualizerAttribute* pAttr = static_cast<const plBoxReflectionProbeVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_vScale.Set(1.0f);
  plVec3 influenceScale;
  plVec3 influenceShift;

  if (!pAttr->GetExtentsProperty().IsEmpty())
  {
    m_vScale = pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetExtentsProperty()));
  }

  if (!pAttr->GetInfluenceScaleProperty().IsEmpty())
  {
    influenceScale = pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetInfluenceScaleProperty()));
  }

  if (!pAttr->GetInfluenceShiftProperty().IsEmpty())
  {
    influenceShift = pObjectAccessor->Get<plVec3>(m_pObject, GetProperty(pAttr->GetInfluenceShiftProperty()));
  }

  m_vPositionOffset = m_vScale.CompMul(influenceShift.CompMul(plVec3(1.0f) - influenceScale)) * 0.5f;
  m_vScale *= influenceScale;

  m_qRotation.SetIdentity();
}

void plBoxReflectionProbeVisualizerAdapter::UpdateGizmoTransform()
{
  plTransform t;
  t.m_vScale = m_vScale;
  t.m_vPosition = m_vPositionOffset;
  t.m_qRotation = m_qRotation;

  m_hGizmo.SetTransformation(GetObjectTransform() * t);
}
