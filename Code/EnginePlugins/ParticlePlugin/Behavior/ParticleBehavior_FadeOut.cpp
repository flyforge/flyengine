#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_FadeOut.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_FadeOut, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_FadeOut>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("StartAlpha", m_fStartAlpha)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("Exponent", m_fExponent)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_FadeOut, 1, plRTTIDefaultAllocator<plParticleBehavior_FadeOut>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const plRTTI* plParticleBehaviorFactory_FadeOut::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_FadeOut>();
}

void plParticleBehaviorFactory_FadeOut::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_FadeOut* pBehavior = static_cast<plParticleBehavior_FadeOut*>(pObject);

  pBehavior->m_fStartAlpha = m_fStartAlpha;
  pBehavior->m_fExponent = m_fExponent;
}

void plParticleBehaviorFactory_FadeOut::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_fStartAlpha;
  inout_stream << m_fExponent;
}

void plParticleBehaviorFactory_FadeOut::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_fStartAlpha;
  inout_stream >> m_fExponent;
}

void plParticleBehavior_FadeOut::CreateRequiredStreams()
{
  CreateStream("LifeTime", plProcessingStream::DataType::Half2, &m_pStreamLifeTime, false);
  CreateStream("Color", plProcessingStream::DataType::Half4, &m_pStreamColor, false);
}

void plParticleBehavior_FadeOut::Process(plUInt64 uiNumElements)
{
  if (!GetOwnerEffect()->IsVisible())
  {
    // set the update interval such that once the effect becomes visible,
    // all particles get fully updated
    m_uiCurrentUpdateInterval = 1;
    m_uiFirstToUpdate = 0;
    return;
  }

  PL_PROFILE_SCOPE("PFX: Fade Out");

  plProcessingStreamIterator<plFloat16Vec2> itLifeTime(m_pStreamLifeTime, uiNumElements, 0);
  plProcessingStreamIterator<plColorLinear16f> itColor(m_pStreamColor, uiNumElements, 0);

  // skip the first n particles
  {
    for (plUInt32 i = 0; i < m_uiFirstToUpdate; ++i)
    {
      itLifeTime.Advance();
      itColor.Advance();
    }

    ++m_uiFirstToUpdate;
    if (m_uiFirstToUpdate >= m_uiCurrentUpdateInterval)
      m_uiFirstToUpdate = 0;
  }

  if (m_fStartAlpha <= 1.0f)
  {
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a = m_fStartAlpha * plMath::Pow(fLifeTimeFraction, m_fExponent);

      for (plUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
      {
        itLifeTime.Advance();
        itColor.Advance();
      }
    }
  }
  else
  {
    // this case has to clamp alpha to 1
    while (!itLifeTime.HasReachedEnd())
    {
      const float fLifeTimeFraction = itLifeTime.Current().x * itLifeTime.Current().y;
      itColor.Current().a = plMath::Min(1.0f, m_fStartAlpha * plMath::Pow(fLifeTimeFraction, m_fExponent));

      for (plUInt32 i = 0; i < m_uiCurrentUpdateInterval; ++i)
      {
        itLifeTime.Advance();
        itColor.Advance();
      }
    }
  }

  /// \todo Use level of detail to reduce the update interval further
  /// up close, with a high interval, animations appear choppy, especially when fading stuff out at the end

  // reset the update interval to the default
  m_uiCurrentUpdateInterval = 2;
}



PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_FadeOut);
