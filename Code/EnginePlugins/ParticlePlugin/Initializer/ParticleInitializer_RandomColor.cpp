#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomColor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory_RandomColor, 1, plRTTIDefaultAllocator<plParticleInitializerFactory_RandomColor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    PL_MEMBER_PROPERTY("Color1", m_Color1)->AddAttributes(new plDefaultValueAttribute(plColor::White), new plExposeColorAlphaAttribute()),
    PL_MEMBER_PROPERTY("Color2", m_Color2)->AddAttributes(new plDefaultValueAttribute(plColor::White), new plExposeColorAlphaAttribute()),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer_RandomColor, 1, plRTTIDefaultAllocator<plParticleInitializer_RandomColor>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleInitializerFactory_RandomColor::GetInitializerType() const
{
  return plGetStaticRTTI<plParticleInitializer_RandomColor>();
}

void plParticleInitializerFactory_RandomColor::CopyInitializerProperties(plParticleInitializer* pInitializer0, bool bFirstTime) const
{
  plParticleInitializer_RandomColor* pInitializer = static_cast<plParticleInitializer_RandomColor*>(pInitializer0);

  pInitializer->m_hGradient = m_hGradient;
  pInitializer->m_Color1 = m_Color1;
  pInitializer->m_Color2 = m_Color2;
}

void plParticleInitializerFactory_RandomColor::SetColorGradientFile(const char* szFile)
{
  plColorGradientResourceHandle hGradient;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hGradient = plResourceManager::LoadResource<plColorGradientResource>(szFile);
  }

  SetColorGradient(hGradient);
}


const char* plParticleInitializerFactory_RandomColor::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void plParticleInitializerFactory_RandomColor::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_hGradient;
  inout_stream << m_Color1;
  inout_stream << m_Color2;
}

void plParticleInitializerFactory_RandomColor::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_hGradient;
  inout_stream >> m_Color1;
  inout_stream >> m_Color2;
}


void plParticleInitializer_RandomColor::CreateRequiredStreams()
{
  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, true);
}

void plParticleInitializer_RandomColor::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PL_PROFILE_SCOPE("PFX: Random Color");

  plColorLinear16f* pColor = m_pStreamColor->GetWritableData<plColorLinear16f>();

  plRandom& rng = GetRNG();

  if (!m_hGradient.IsValid())
  {
    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float f = (float)rng.DoubleZeroToOneInclusive();
      pColor[i] = plMath::Lerp(m_Color1, m_Color2, f);
    }
  }
  else
  {
    plResourceLock<plColorGradientResource> pResource(m_hGradient, plResourceAcquireMode::BlockTillLoaded);

    double fMinValue, fMaxValue;
    const plColorGradient& gradient = pResource->GetDescriptor().m_Gradient;
    gradient.GetExtents(fMinValue, fMaxValue);

    plColorGammaUB color;
    float intensity;

    const bool bMulColor = (m_Color1 != plColor::White) && (m_Color2 != plColor::White);

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const double f = rng.DoubleMinMax(fMinValue, fMaxValue);

      gradient.Evaluate(f, color, intensity);

      plColor result = color;
      result.r *= intensity;
      result.g *= intensity;
      result.b *= intensity;

      if (bMulColor)
      {
        const float f2 = (float)rng.DoubleZeroToOneInclusive();
        result *= plMath::Lerp(m_Color1, m_Color2, f2);
      }

      pColor[i] = result;
    }
  }
}



PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomColor);
