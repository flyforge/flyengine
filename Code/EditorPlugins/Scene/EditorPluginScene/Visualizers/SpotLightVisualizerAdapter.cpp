#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plSpotLightVisualizerAdapter::plSpotLightVisualizerAdapter() {}

plSpotLightVisualizerAdapter::~plSpotLightVisualizerAdapter() {}

void plSpotLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PLASMA_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, PlasmaEngineGizmoHandleType::Cone, plColor::White, plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plSpotLightVisualizerAdapter::Update()
{
  const plSpotLightVisualizerAttribute* pAttr = static_cast<const plSpotLightVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_fAngleScale = 1.0f;

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plSpotLightVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<plColor>());
  }

  m_fScale = 1.0f;
  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty() && !pAttr->GetAngleProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value).IgnoreResult();

    PLASMA_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plAngle>(), "Invalid property bound to plSpotLightVisualizerAttribute 'angle'");
    m_fAngleScale = plMath::Tan(value.ConvertTo<plAngle>() * 0.5f);

    plVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).IgnoreResult();
    PLASMA_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to plPointLightVisualizerAttribute 'radius'");

    plVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).IgnoreResult();
    PLASMA_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to plPointLightVisualizerAttribute 'intensity'");

    m_fScale = plMath::Cos(value.ConvertTo<plAngle>() * 0.5) * range.ConvertTo<float>();
  }

  m_hGizmo.SetVisible(m_fAngleScale != 0.0f && m_fScale != 0.0f);
}

void plSpotLightVisualizerAdapter::UpdateGizmoTransform()
{
  plTransform t = GetObjectTransform();
  t.m_vScale *= plVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fScale;
  m_hGizmo.SetTransformation(t);
}
