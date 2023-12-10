#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_ColorGradient, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_ColorGradient>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Gradient", GetColorGradientFile, SetColorGradientFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Data_Gradient")),
    PLASMA_MEMBER_PROPERTY("TintColor", m_TintColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PLASMA_ENUM_MEMBER_PROPERTY("ColorGradientMode", plParticleColorGradientMode, m_GradientMode),
    PLASMA_MEMBER_PROPERTY("GradientMaxSpeed", m_fMaxSpeed)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 100.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_ColorGradient, 1, plRTTIDefaultAllocator<plParticleBehavior_ColorGradient>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleBehaviorFactory_ColorGradient::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_ColorGradient>();
}

void plParticleBehaviorFactory_ColorGradient::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_ColorGradient* pBehavior = static_cast<plParticleBehavior_ColorGradient*>(pObject);

  pBehavior->m_hGradient = m_hGradient;
  pBehavior->m_GradientMode = m_GradientMode;
  pBehavior->m_fMaxSpeed = m_fMaxSpeed;
  pBehavior->m_TintColor = m_TintColor;

  // the gradient resource may not be specified yet, so defer evaluation until an element is created
  pBehavior->m_InitColor = plColor::RebeccaPurple;
}

void plParticleBehaviorFactory_ColorGradient::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = 4;
  stream << uiVersion;

  stream << m_hGradient;

  // version 3
  stream << m_GradientMode;
  stream << m_fMaxSpeed;

  // Version 4
  stream << m_TintColor;
}

void plParticleBehaviorFactory_ColorGradient::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  stream >> m_hGradient;

  if (uiVersion >= 3)
  {
    stream >> m_GradientMode;
    stream >> m_fMaxSpeed;
  }

  if (uiVersion >= 4)
  {
    stream >> m_TintColor;
  }
}

void plParticleBehaviorFactory_ColorGradient::SetColorGradientFile(const char* szFile)
{
  plColorGradientResourceHandle hGradient;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hGradient = plResourceManager::LoadResource<plColorGradientResource>(szFile);
  }

  SetColorGradient(hGradient);
}

const char* plParticleBehaviorFactory_ColorGradient::GetColorGradientFile() const
{
  if (!m_hGradient.IsValid())
    return "";

  return m_hGradient.GetResourceID();
}

void plParticleBehavior_ColorGradient::CreateRequiredStreams()
{
  m_pStreamColor = nullptr;
  m_pStreamVelocity = nullptr;

  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, false);

  if (m_GradientMode == plParticleColorGradientMode::Age)
  {
    CreateStream("LifeTime", plProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  }
  else if (m_GradientMode == plParticleColorGradientMode::Speed)
  {
    CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
  }
}

void plParticleBehavior_ColorGradient::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  if (!m_hGradient.IsValid())
    return;

  PLASMA_PROFILE_SCOPE("PFX: Color Gradient Init");

  // query the init color from the gradient
  if (m_InitColor == plColor::RebeccaPurple)
  {
    m_InitColor = m_TintColor;

    plResourceLock<plColorGradientResource> pGradient(m_hGradient, plResourceAcquireMode::BlockTillLoaded);

    if (pGradient.GetAcquireResult() != plResourceAcquireResult::MissingFallback)
    {
      const plColorGradient& gradient = pGradient->GetDescriptor().m_Gradient;

      plColor rgba;
      plUInt8 alpha;
      gradient.EvaluateColor(0, rgba);
      gradient.EvaluateAlpha(0, alpha);
      rgba.a = plMath::ColorByteToFloat(alpha);

      m_InitColor = rgba;
    }
  }

  const plColorLinear16f initCol16 = m_InitColor;

  plProcessingStreamIterator<plColorLinear16f> itColor(m_pStreamColor, uiNumElements, uiStartIndex);
  while (!itColor.HasReachedEnd())
  {
    itColor.Current() = initCol16;
    itColor.Advance();
  }
}

void plParticleBehavior_ColorGradient::Process(plUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // set the update interval such that once the effect becomes visible,
    // all particles get fully updated
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  if (!m_hGradient.IsValid())
    return;

  PLASMA_PROFILE_SCOPE("PFX: Color Gradient");

  plResourceLock<plColorGradientResource> pGradient(m_hGradient, plResourceAcquireMode::BlockTillLoaded);

  if (pGradient.GetAcquireResult() == plResourceAcquireResult::MissingFallback)
    return;

  const plColorGradient& gradient = pGradient->GetDescriptor().m_Gradient;

  plProcessingStreamIterator<plColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  itColor.Advance(m_uiFirstToUpdate);

  if (m_GradientMode == plParticleColorGradientMode::Age)
  {
    plProcessingStreamIterator<plFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);

    // skip the first n particles
    itLifeTime.Advance(m_uiFirstToUpdate);

    while (!itLifeTime.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
        const float posx = 1.0f - fLifeTimeFraction;

        plColor rgba;
        plUInt8 alpha;
        gradient.EvaluateColor(posx, rgba);
        gradient.EvaluateAlpha(posx, alpha);
        rgba.a = plMath::ColorByteToFloat(alpha);

        itColor.Current() = rgba * m_TintColor;
      }

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated,
      // since sampling the color gradient is pretty expensive
      itLifeTime.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }
  else if (m_GradientMode == plParticleColorGradientMode::Speed)
  {
    plProcessingStreamIterator<plVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

    // skip the first n particles
    itVelocity.Advance(m_uiFirstToUpdate);

    while (!itVelocity.HasReachedEnd())
    {
      // if (itLifeTime.Current().y > 0)
      {
        const float fSpeed = itVelocity.Current().GetLength();
        const float posx = fSpeed / m_fMaxSpeed; // no need to clamp the range, the color lookup will already do that

        plColor rgba;
        plUInt8 alpha;
        gradient.EvaluateColor(posx, rgba);
        gradient.EvaluateAlpha(posx, alpha);
        rgba.a = plMath::ColorByteToFloat(alpha);

        itColor.Current() = rgba * m_TintColor;
      }

      // skip the next n items
      // this is to reduce the number of particles that need to be fully evaluated,
      // since sampling the color gradient is pretty expensive
      itVelocity.Advance(m_uiCurrentUpdateInterval);
      itColor.Advance(m_uiCurrentUpdateInterval);
    }
  }

  // adjust which index is the first to update
  {
    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_ColorGradient);
