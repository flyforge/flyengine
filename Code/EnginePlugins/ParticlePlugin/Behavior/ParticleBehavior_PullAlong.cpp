#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_PullAlong.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_PullAlong, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_PullAlong>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_PullAlong, 1, plRTTIDefaultAllocator<plParticleBehavior_PullAlong>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleBehaviorFactory_PullAlong::plParticleBehaviorFactory_PullAlong() {}

const plRTTI* plParticleBehaviorFactory_PullAlong::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_PullAlong>();
}

void plParticleBehaviorFactory_PullAlong::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_PullAlong* pBehavior = static_cast<plParticleBehavior_PullAlong*>(pObject);

  pBehavior->m_fStrength = plMath::Clamp(m_fStrength, 0.0f, 1.0f);
}

enum class BehaviorPullAlongVersion
{
  Version_0 = 0,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleBehaviorFactory_PullAlong::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)BehaviorPullAlongVersion::Version_Current;
  stream << uiVersion;

  stream << m_fStrength;
}

void plParticleBehaviorFactory_PullAlong::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)BehaviorPullAlongVersion::Version_Current, "Invalid version {0}", uiVersion);

  stream >> m_fStrength;
}

void plParticleBehavior_PullAlong::CreateRequiredStreams()
{
  m_bFirstTime = true;
  m_vApplyPull.SetZero();

  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
}

void plParticleBehavior_PullAlong::Process(plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: PullAlong");

  if (m_vApplyPull.IsZero())
    return;

  plProcessingStreamIterator<plSimdVec4f> itPosition(m_pStreamPosition, uiNumElements, 0);
  plSimdVec4f pull;
  pull.Load<3>(&m_vApplyPull.x);

  while (!itPosition.HasReachedEnd())
  {
    itPosition.Current() += pull;

    itPosition.Advance();
  }
}

void plParticleBehavior_PullAlong::StepParticleSystem(const plTime& tDiff, plUInt32 uiNumNewParticles)
{
  const plVec3 vPos = GetOwnerSystem()->GetTransform().m_vPosition;

  if (!m_bFirstTime)
  {
    m_vApplyPull = (vPos - m_vLastEmitterPosition) * m_fStrength;
  }
  else
  {
    m_bFirstTime = false;
    m_vApplyPull.SetZero();
  }

  m_vLastEmitterPosition = vPos;
}
