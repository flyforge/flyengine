#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorPluginScene/Visualizers/PointLightVisualizerAdapter.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plPointLightVisualizerAdapter::plPointLightVisualizerAdapter() = default;

plPointLightVisualizerAdapter::~plPointLightVisualizerAdapter() = default;

void plPointLightVisualizerAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();
  const plAssetDocument* pAssetDocument = plDynamicCast<const plAssetDocument*>(pDoc);
  PL_ASSERT_DEV(pAssetDocument != nullptr, "Visualizers are only supported in plAssetDocument.");

  m_hGizmo.ConfigureHandle(nullptr, plEngineGizmoHandleType::Sphere, plColor::White, plGizmoFlags::ShowInOrtho | plGizmoFlags::Visualizer);

  pAssetDocument->AddSyncObject(&m_hGizmo);
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
}

void plPointLightVisualizerAdapter::Update()
{
  m_hGizmo.SetVisible(m_bVisualizerIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plPointLightVisualizerAttribute* pAttr = static_cast<const plPointLightVisualizerAttribute*>(m_pVisualizerAttr);

  m_fScale = 1.0f;

  if (!pAttr->GetRangeProperty().IsEmpty() && !pAttr->GetIntensityProperty().IsEmpty())
  {
    plVariant range;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetRangeProperty()), range).AssertSuccess();
    PL_ASSERT_DEBUG(range.CanConvertTo<float>(), "Invalid property bound to plPointLightVisualizerAttribute 'radius'");

    plVariant intensity;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetIntensityProperty()), intensity).AssertSuccess();
    PL_ASSERT_DEBUG(intensity.CanConvertTo<float>(), "Invalid property bound to plPointLightVisualizerAttribute 'intensity'");

    m_fScale = plLightComponent::CalculateEffectiveRange(range.ConvertTo<float>(), intensity.ConvertTo<float>());
  }

  if (!pAttr->GetColorProperty().IsEmpty())
  {
    plVariant value;
    pObjectAccessor->GetValue(m_pObject, GetProperty(pAttr->GetColorProperty()), value).AssertSuccess();

    PL_ASSERT_DEBUG(value.IsValid() && value.CanConvertTo<plColor>(), "Invalid property bound to plPointLightVisualizerAdapter 'color'");
    m_hGizmo.SetColor(value.ConvertTo<plColor>());
  }
}

void plPointLightVisualizerAdapter::UpdateGizmoTransform()
{
  plTransform t = GetObjectTransform();
  t.m_vScale *= m_fScale;

  m_hGizmo.SetTransformation(t);
}
