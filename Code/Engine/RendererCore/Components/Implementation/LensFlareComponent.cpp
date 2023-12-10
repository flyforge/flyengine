#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/LensFlareComponent.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLensFlareRenderData, 1, plRTTIDefaultAllocator<plLensFlareRenderData>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLensFlareRenderData::FillBatchIdAndSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const plUInt32 uiTextureIDHash = static_cast<plUInt32>(m_hTexture.GetResourceIDHash());

  // Batch and sort by texture
  m_uiBatchId = uiTextureIDHash;
  m_uiSortingKey = uiTextureIDHash;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plLensFlareElement, plNoBase, 1, plRTTIDefaultAllocator<plLensFlareElement>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    PLASMA_MEMBER_PROPERTY("GreyscaleTexture", m_bGreyscaleTexture),
    PLASMA_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new plExposeColorAlphaAttribute()),
    PLASMA_MEMBER_PROPERTY("ModulateByLightColor", m_bModulateByLightColor)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(10000.0f), new plSuffixAttribute(" m")),
    PLASMA_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("ShiftToCenter", m_fShiftToCenter),
    PLASMA_MEMBER_PROPERTY("InverseTonemap", m_bInverseTonemap),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plLensFlareElement::SetTextureFile(const char* szFile)
{
  plTexture2DResourceHandle hTexture;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = plResourceManager::LoadResource<plTexture2DResource>(szFile);
  }

  m_hTexture = hTexture;
}

const char* plLensFlareElement::GetTextureFile() const
{
  if (!m_hTexture.IsValid())
    return "";

  return m_hTexture.GetResourceID();
}

plResult plLensFlareElement::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_hTexture;
  inout_stream << m_Color;
  inout_stream << m_fSize;
  inout_stream << m_fMaxScreenSize;
  inout_stream << m_fAspectRatio;
  inout_stream << m_fShiftToCenter;
  inout_stream << m_bInverseTonemap;
  inout_stream << m_bModulateByLightColor;
  inout_stream << m_bGreyscaleTexture;

  return PLASMA_SUCCESS;
}

plResult plLensFlareElement::Deserialize(plStreamReader& inout_stream)
{
  inout_stream >> m_hTexture;
  inout_stream >> m_Color;
  inout_stream >> m_fSize;
  inout_stream >> m_fMaxScreenSize;
  inout_stream >> m_fAspectRatio;
  inout_stream >> m_fShiftToCenter;
  inout_stream >> m_bInverseTonemap;
  inout_stream >> m_bModulateByLightColor;
  inout_stream >> m_bGreyscaleTexture;

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plLensFlareComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("LinkToLightShape", GetLinkToLightShape, SetLinkToLightShape)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PLASMA_ACCESSOR_PROPERTY("OcclusionSampleRadius", GetOcclusionSampleRadius, SetOcclusionSampleRadius)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(0.1f), new plSuffixAttribute(" m")),
    PLASMA_MEMBER_PROPERTY("OcclusionSampleSpread", m_fOcclusionSampleSpread)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f), new plDefaultValueAttribute(0.5f)),
    PLASMA_MEMBER_PROPERTY("OcclusionDepthOffset", m_fOcclusionDepthOffset)->AddAttributes(new plSuffixAttribute(" m")),
    PLASMA_MEMBER_PROPERTY("ApplyFog", m_bApplyFog)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ARRAY_MEMBER_PROPERTY("Elements", m_Elements)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
    new plSphereManipulatorAttribute("OcclusionSampleRadius"),
    new plSphereVisualizerAttribute("OcclusionSampleRadius", plColor::White)
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plLensFlareComponent::plLensFlareComponent() = default;
plLensFlareComponent::~plLensFlareComponent() = default;

void plLensFlareComponent::OnActivated()
{
  SUPER::OnActivated();

  FindLightComponent();
}

void plLensFlareComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_bDirectionalLight = false;
  m_hLightComponent.Invalidate();
}

void plLensFlareComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s.WriteArray(m_Elements).IgnoreResult();
  s << m_fIntensity;
  s << m_fOcclusionSampleRadius;
  s << m_fOcclusionSampleSpread;
  s << m_fOcclusionDepthOffset;
  s << m_bLinkToLightShape;
  s << m_bApplyFog;
}

void plLensFlareComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  PLASMA_IGNORE_UNUSED(uiVersion);

  plStreamReader& s = inout_stream.GetStream();

  s.ReadArray(m_Elements).IgnoreResult();
  s >> m_fIntensity;
  s >> m_fOcclusionSampleRadius;
  s >> m_fOcclusionSampleSpread;
  s >> m_fOcclusionDepthOffset;
  s >> m_bLinkToLightShape;
  s >> m_bApplyFog;
}

plResult plLensFlareComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_bDirectionalLight)
  {
    ref_bAlwaysVisible = true;
  }
  else
  {
    ref_bounds = plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fOcclusionSampleRadius);
  }
  return PLASMA_SUCCESS;
}

void plLensFlareComponent::SetLinkToLightShape(bool bLink)
{
  if (m_bLinkToLightShape == bLink)
    return;

  m_bLinkToLightShape = bLink;
  if (IsActiveAndInitialized())
  {
    FindLightComponent();
  }

  TriggerLocalBoundsUpdate();
}

void plLensFlareComponent::SetOcclusionSampleRadius(float fRadius)
{
  m_fOcclusionSampleRadius = fRadius;

  TriggerLocalBoundsUpdate();
}

void plLensFlareComponent::FindLightComponent()
{
  plLightComponent* pLightComponent = nullptr;

  if (m_bLinkToLightShape)
  {
    plGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      if (pObject->TryGetComponentOfBaseType(pLightComponent))
        break;

      pObject = pObject->GetParent();
    }
  }

  if (pLightComponent != nullptr)
  {
    m_bDirectionalLight = pLightComponent->IsInstanceOf<plDirectionalLightComponent>();
    m_hLightComponent = pLightComponent->GetHandle();
  }
  else
  {
    m_bDirectionalLight = false;
    m_hLightComponent.Invalidate();
  }
}

void plLensFlareComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  const plCamera* pCamera = msg.m_pView->GetCamera();
  plTransform globalTransform = GetOwner()->GetGlobalTransform();
  plBoundingBoxSphere globalBounds = GetOwner()->GetGlobalBounds();
  float fScale = globalTransform.GetMaxScale();
  plColor lightColor = plColor::White;

  const plLightComponent* pLightComponent = nullptr;
  if (GetWorld()->TryGetComponent(m_hLightComponent, pLightComponent))
  {
    lightColor = pLightComponent->GetLightColor();
    lightColor *= pLightComponent->GetIntensity() * 0.1f;
  }

  float fFade = 1.0f;
  if (auto pDirectionalLight = plDynamicCast<const plDirectionalLightComponent*>(pLightComponent))
  {
    plTransform localOffset = plTransform::MakeIdentity();
    localOffset.m_vPosition = plVec3(pCamera->GetFarPlane() * -0.999, 0, 0);

    globalTransform = plTransform::MakeGlobalTransform(globalTransform, localOffset);
    globalTransform.m_vPosition += pCamera->GetCenterPosition();

    if (pCamera->IsPerspective())
    {
      float fHalfHeight = plMath::Tan(pCamera->GetFovY(1.0f) * 0.5f) * pCamera->GetFarPlane();
      fScale *= fHalfHeight;
    }

    lightColor *= 10.0f;
  }
  else if (auto pSpotLight = plDynamicCast<const plSpotLightComponent*>(pLightComponent))
  {
    const plVec3 lightDir = globalTransform.TransformDirection(plVec3::MakeAxisX());
    const plVec3 cameraDir = (pCamera->GetCenterPosition() - globalTransform.m_vPosition).GetNormalized();

    const float cosAngle = lightDir.Dot(cameraDir);
    const float fCosInner = plMath::Cos(pSpotLight->GetInnerSpotAngle() * 0.5f);
    const float fCosOuter = plMath::Cos(pSpotLight->GetOuterSpotAngle() * 0.5f);
    fFade = plMath::Saturate((cosAngle - fCosOuter) / plMath::Max(0.001f, (fCosInner - fCosOuter)));
    fFade *= fFade;
  }

  for (auto& element : m_Elements)
  {
    if (element.m_hTexture.IsValid() == false)
      continue;

    plColor color = element.m_Color * m_fIntensity;
    if (element.m_bModulateByLightColor)
    {
      color *= lightColor;
    }
    color.a = element.m_Color.a * fFade;

    if (color.GetLuminance() <= 0.0f || color.a <= 0.0f)
      continue;

    plLensFlareRenderData* pRenderData = plCreateRenderDataForThisFrame<plLensFlareRenderData>(GetOwner());
    {
      pRenderData->m_GlobalTransform = globalTransform;
      pRenderData->m_GlobalBounds = globalBounds;
      pRenderData->m_hTexture = element.m_hTexture;
      pRenderData->m_Color = color.GetAsVec4();
      pRenderData->m_fSize = element.m_fSize * fScale;
      pRenderData->m_fMaxScreenSize = element.m_fMaxScreenSize * 2.0f;
      pRenderData->m_fAspectRatio = 1.0f / element.m_fAspectRatio;
      pRenderData->m_fShiftToCenter = element.m_fShiftToCenter;
      pRenderData->m_fOcclusionSampleRadius = m_fOcclusionSampleRadius * fScale;
      pRenderData->m_fOcclusionSampleSpread = m_fOcclusionSampleSpread;
      pRenderData->m_fOcclusionDepthOffset = m_fOcclusionDepthOffset * fScale;
      pRenderData->m_bInverseTonemap = element.m_bInverseTonemap;
      pRenderData->m_bGreyscaleTexture = element.m_bGreyscaleTexture;
      pRenderData->m_bApplyFog = m_bApplyFog;

      pRenderData->FillBatchIdAndSortingKey();
    }

    msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::LitTransparent,
      pLightComponent != nullptr ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic);
  }
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LensFlareComponent);
