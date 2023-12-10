#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_SizeCurve.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_SizeCurve, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_SizeCurve>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("SizeCurve", GetSizeCurveFile, SetSizeCurveFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Curve")),
    PLASMA_MEMBER_PROPERTY("BaseSize", m_fBaseSize)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("CurveScale", m_fCurveScale)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_SizeCurve, 1, plRTTIDefaultAllocator<plParticleBehavior_SizeCurve>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleBehaviorFactory_SizeCurve::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_SizeCurve>();
}

void plParticleBehaviorFactory_SizeCurve::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_SizeCurve* pBehavior = static_cast<plParticleBehavior_SizeCurve*>(pObject);

  pBehavior->m_hCurve = m_hCurve;
  pBehavior->m_fBaseSize = m_fBaseSize;
  pBehavior->m_fCurveScale = m_fCurveScale;
}

void plParticleBehaviorFactory_SizeCurve::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_hCurve;
  stream << m_fBaseSize;
  stream << m_fCurveScale;
}

void plParticleBehaviorFactory_SizeCurve::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hCurve;
  stream >> m_fBaseSize;
  stream >> m_fCurveScale;
}

void plParticleBehaviorFactory_SizeCurve::SetSizeCurveFile(const char* szFile)
{
  plCurve1DResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plCurve1DResource>(szFile);
  }

  m_hCurve = hResource;
}


const char* plParticleBehaviorFactory_SizeCurve::GetSizeCurveFile() const
{
  if (!m_hCurve.IsValid())
    return "";

  return m_hCurve.GetResourceID();
}

void plParticleBehavior_SizeCurve::CreateRequiredStreams()
{
  CreateStream("LifeTime", plProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Size", plProcessingStream::DataType::Half, &m_pStreamSize, false);
}


void plParticleBehavior_SizeCurve::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  plProcessingStreamIterator<plFloat16> itSize(m_pStreamSize, uiNumElements, uiStartIndex);
  while (!itSize.HasReachedEnd())
  {
    itSize.Current() = m_fBaseSize;
    itSize.Advance();
  }
}

void plParticleBehavior_SizeCurve::Process(plUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // reduce the update interval when the effect is not visible
    m_uiCurrentUpdateInterval = 32;
  }
  else
  {
    m_uiCurrentUpdateInterval = 2;
  }

  if (!m_hCurve.IsValid())
    return;

  PLASMA_PROFILE_SCOPE("PFX: Size Curve");

  plProcessingStreamIterator<plFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  plProcessingStreamIterator<plFloat16> itSize(m_pStreamSize, uiNumElements, 0);

  plResourceLock<plCurve1DResource> pCurve(m_hCurve, plResourceAcquireMode::BlockTillLoaded);

  if (pCurve.GetAcquireResult() == plResourceAcquireResult::MissingFallback)
    return;

  if (pCurve->GetDescriptor().m_Curves.IsEmpty())
    return;

  auto& curve = pCurve->GetDescriptor().m_Curves[0];

  double fMinX, fMaxX;
  curve.QueryExtents(fMinX, fMaxX);

  // skip the first n particles
  {
    for (plUInt32 i = 0; i < m_uiFirstToUpdate; ++i)
    {
      itLifeTime.Advance();
      itSize.Advance();
    }

    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  while (!itLifeTime.HasReachedEnd())
  {
    // if (itLifeTime.Current().y > 0)
    {
      const float fLifeTimeFraction = 1.0f - (itLifeTime.Current().x * itLifeTime.Current().y);

      const double evalPos = curve.ConvertNormalizedPos(fLifeTimeFraction);
      double val = curve.Evaluate(evalPos);
      val = curve.NormalizeValue(val);

      itSize.Current() = m_fBaseSize + (float)val * m_fCurveScale;
    }

    // skip the next n items
    // this is to reduce the number of particles that need to be fully evaluated,
    // since sampling the curve is expensive
    for (plUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
    {
      itLifeTime.Advance();
      itSize.Advance();
    }
  }
}



PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_SizeCurve);
