#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/SpotLightVisualizerAdapter.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plSpotLightVisualizerAdapter::plSpotLightVisualizerAdapter() = default;

plSpotLightVisualizerAdapter::~plSpotLightVisualizerAdapter() = default;

void plSpotLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PL_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, plEngineGizmoHandleType::Cone, plColor::White, plGizmoFlags::ShowInOrtho | plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plSpotLightVisualizerAdapter::Update()
{
  const plSpotLightVisualizerAttribute* pAttr = static_cast<const plSpotLightVisualizerAttribute*>(m_pVisualizerAttr);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);

  m_fAngleScale = 1.0f;
  if (!pAttr->GetAngleProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetAngleProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plAngle>(), "Invalid property bound to plSpotLightVisualizerAttribute 'angle'");
    m_fAngleScale = plMath::Tan(value.ConvertTo<plAngle>() * 0.5f);
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plSpotLightVisualizerAttribute 'color'");
    m_hGizmo.SetColor(value.ConvertTo<plColor>());
  }

  m_fScale = 1.0f;
  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    plVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).AssertSuccess();
    PL_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to plPointLightVisualizerAttribute 'radius'");

    plVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).AssertSuccess();
    PL_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to plPointLightVisualizerAttribute 'intensity'");

    m_fScale = range.ConvertTo<float>();//plLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  m_hGizmo.SetVisible(m_fAngleScale != 0.0f && m_fScale != 0.0f);
}

void plSpotLightVisualizerAdapter::UpdateGizmoTransform()
{
  plTransform t = GetObjectTransform();
  t.m_vScale = t.m_vScale.CompMul(plVec3(1.0f, m_fAngleScale, m_fAngleScale) * m_fScale);
  m_hGizmo.SetTransformation(t);
}
