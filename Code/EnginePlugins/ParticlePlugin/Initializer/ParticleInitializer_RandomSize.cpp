#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_RandomSize.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory_RandomSize, 2, plRTTIDefaultAllocator<plParticleInitializerFactory_RandomSize>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Size", m_Size)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("SizeCurve", GetSizeCurveFile, SetSizeCurveFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer_RandomSize, 1, plRTTIDefaultAllocator<plParticleInitializer_RandomSize>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleInitializerFactory_RandomSize::GetInitializerType() const
{
  return plGetStaticRTTI<plParticleInitializer_RandomSize>();
}

void plParticleInitializerFactory_RandomSize::CopyInitializerProperties(plParticleInitializer* pInitializer0, bool bFirstTime) const
{
  plParticleInitializer_RandomSize* pInitializer = static_cast<plParticleInitializer_RandomSize*>(pInitializer0);

  pInitializer->m_hCurve = m_hCurve;
  pInitializer->m_Size = m_Size;
}

void plParticleInitializerFactory_RandomSize::SetSizeCurveFile(const char* szFile)
{
  plCurve1DResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plCurve1DResource>(szFile);
  }

  m_hCurve = hResource;
}


const char* plParticleInitializerFactory_RandomSize::GetSizeCurveFile() const
{
  if (!m_hCurve.IsValid())
    return "";

  return m_hCurve.GetResourceID();
}

void plParticleInitializerFactory_RandomSize::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 2;
  inout_stream << uiVersion;

  inout_stream << m_hCurve;
  inout_stream << m_Size.m_Value;
  inout_stream << m_Size.m_fVariance;
}

void plParticleInitializerFactory_RandomSize::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_hCurve;
  inout_stream >> m_Size.m_Value;
  inout_stream >> m_Size.m_fVariance;
}


void plParticleInitializer_RandomSize::CreateRequiredStreams()
{
  CreateStream("Size", plProcessingStream::DataType::Half, &m_pStreamSize, true);
}

void plParticleInitializer_RandomSize::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PL_PROFILE_SCOPE("PFX: Random Size");

  plFloat16* pSize = m_pStreamSize->GetWritableData<plFloat16>();

  plRandom& rng = GetRNG();

  if (!m_hCurve.IsValid())
  {
    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pSize[i] = (float)rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance);
    }
  }
  else
  {
    plResourceLock<plCurve1DResource> pResource(m_hCurve, plResourceAcquireMode::BlockTillLoaded);

    if (!pResource->GetDescriptor().m_Curves.IsEmpty())
    {
      const plCurve1D& curve = pResource->GetDescriptor().m_Curves[0];

      double fMinX, fMaxX;
      curve.QueryExtents(fMinX, fMaxX);

      for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        const double f = rng.DoubleMinMax(fMinX, fMaxX);

        double val = curve.Evaluate(f);
        val = curve.NormalizeValue(val);

        pSize[i] = (float)(val * rng.DoubleVariance(m_Size.m_Value, m_Size.m_fVariance));
      }
    }
    else
    {
      for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
      {
        pSize[i] = 1.0f;
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class plParticleInitializerFactory_RandomSize_1_2 : public plGraphPatch
{
public:
  plParticleInitializerFactory_RandomSize_1_2()
    : plGraphPatch("plParticleInitializerFactory_RandomSize", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Size").IgnoreResult();
  }
};

plParticleInitializerFactory_RandomSize_1_2 g_plParticleInitializerFactory_RandomSize_1_2;

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_RandomSize);
