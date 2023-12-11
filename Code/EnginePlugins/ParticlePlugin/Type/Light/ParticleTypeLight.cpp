#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Type/Light/ParticleTypeLight.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeLightFactory, 1, plRTTIDefaultAllocator<plParticleTypeLightFactory>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("SizeFactor", m_fSizeFactor)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.0f, 1000.0f)),
    PLASMA_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 100000.0f)),
    PLASMA_MEMBER_PROPERTY("Percentage", m_uiPercentage)->AddAttributes(new plDefaultValueAttribute(50), new plClampValueAttribute(1, 100)),
    PLASMA_MEMBER_PROPERTY("TintColorParam", m_sTintColorParameter),
    PLASMA_MEMBER_PROPERTY("IntensityScaleParam", m_sIntensityParameter),
    PLASMA_MEMBER_PROPERTY("SizeScaleParam", m_sSizeScaleParameter),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleTypeLight, 1, plRTTIDefaultAllocator<plParticleTypeLight>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleTypeLightFactory::plParticleTypeLightFactory()
{
  m_fSizeFactor = 5.0f;
  m_fIntensity = 10.0f;
  m_uiPercentage = 50;
}


const plRTTI* plParticleTypeLightFactory::GetTypeType() const
{
  return plGetStaticRTTI<plParticleTypeLight>();
}

void plParticleTypeLightFactory::CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const
{
  plParticleTypeLight* pType = static_cast<plParticleTypeLight*>(pObject);

  pType->m_fSizeFactor = m_fSizeFactor;
  pType->m_fIntensity = m_fIntensity;
  pType->m_uiPercentage = m_uiPercentage;
  pType->m_sTintColorParameter = plTempHashedString(m_sTintColorParameter.GetData());
  pType->m_sIntensityParameter = plTempHashedString(m_sIntensityParameter.GetData());
  pType->m_sSizeScaleParameter = plTempHashedString(m_sSizeScaleParameter.GetData());
}

enum class TypeLightVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added tint color and intensity parameter

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleTypeLightFactory::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)TypeLightVersion::Version_Current;
  stream << uiVersion;

  stream << m_fSizeFactor;
  stream << m_fIntensity;
  stream << m_uiPercentage;

  // Version 2
  stream << m_sTintColorParameter;
  stream << m_sIntensityParameter;
  stream << m_sSizeScaleParameter;
}

void plParticleTypeLightFactory::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)TypeLightVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fSizeFactor;
  stream >> m_fIntensity;
  stream >> m_uiPercentage;

  if (uiVersion >= 2)
  {
    stream >> m_sTintColorParameter;
    stream >> m_sIntensityParameter;
    stream >> m_sSizeScaleParameter;
  }
}

void plParticleTypeLight::CreateRequiredStreams()
{
  m_pStreamOnOff = nullptr;

  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Size", plProcessingStream::DataType::Half, &m_pStreamSize, false);
  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, false);

  if (m_uiPercentage < 100)
  {
    CreateStream("OnOff", plProcessingStream::DataType::Int, &m_pStreamOnOff, false); /// \todo Initialize (instead of during extraction)
  }
}


void plParticleTypeLight::ExtractTypeRenderData(plMsgExtractRenderData& msg, const plTransform& instanceTransform) const
{
  PLASMA_PROFILE_SCOPE("PFX: Light");

  const plVec4* pPosition = m_pStreamPosition->GetData<plVec4>();
  const plFloat16* pSize = m_pStreamSize->GetData<plFloat16>();
  const plColorLinear16f* pColor = m_pStreamColor->GetData<plColorLinear16f>();

  if (pPosition == nullptr || pSize == nullptr || pColor == nullptr)
    return;

  plInt32* pOnOff = nullptr;

  if (m_pStreamOnOff)
  {
    pOnOff = m_pStreamOnOff->GetWritableData<plInt32>();

    if (pOnOff == nullptr)
      return;
  }

  plRandom& rng = GetRNG();

  const plUInt32 uiNumParticles = (plUInt32)GetOwnerSystem()->GetNumActiveParticles();

  const plUInt32 uiBatchId = 1; // no shadows

  const plColor tintColor = GetOwnerEffect()->GetColorParameter(m_sTintColorParameter, plColor::White);
  const float intensityScale = GetOwnerEffect()->GetFloatParameter(m_sIntensityParameter, 1.0f);
  const float sizeScale = GetOwnerEffect()->GetFloatParameter(m_sSizeScaleParameter, 1.0f);

  const float sizeFactor = m_fSizeFactor * sizeScale;
  const float intensity = intensityScale * m_fIntensity;

  plTransform transform;

  if (this->GetOwnerEffect()->IsSimulatedInLocalSpace())
    transform = instanceTransform;
  else
    transform.SetIdentity();

  for (plUInt32 i = 0; i < uiNumParticles; ++i)
  {
    if (pOnOff)
    {
      if (pOnOff[i] == 0)
      {
        if ((plUInt32)rng.IntMinMax(0, 100) <= m_uiPercentage)
          pOnOff[i] = 1;
        else
          pOnOff[i] = -1;
      }

      if (pOnOff[i] < 0)
        continue;
    }

    auto pRenderData = plCreateRenderDataForThisFrame<plPointLightRenderData>(nullptr);

    pRenderData->m_GlobalTransform.SetIdentity();
    pRenderData->m_GlobalTransform.m_vPosition = transform * pPosition[i].GetAsVec3();
    pRenderData->m_LightColor = tintColor * pColor[i].ToLinearFloat();
    pRenderData->m_fIntensity = intensity;
//    pRenderData->m_fFalloff = 0.8f;
//    pRenderData->m_fSpecularMultiplier = 1.0;
//    pRenderData->m_fVolumetricIntensity = 0.0f; //TODO: add volumetric intensity to the particle type
    pRenderData->m_fRange = pSize[i] * sizeFactor;
    pRenderData->m_uiShadowDataOffset = plInvalidIndex;

    float fScreenSpaceSize = plLightComponent::CalculateScreenSpaceSize(plBoundingSphere::MakeFromCenterAndRadius(pRenderData->m_GlobalTransform.m_vPosition, pRenderData->m_fRange * 0.5f), *msg.m_pView->GetCullingCamera());
    pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

    msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::Light, plRenderData::Caching::Never);
  }
}



PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Type_Light_ParticleTypeLight);