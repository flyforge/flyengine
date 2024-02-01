#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Flies.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_Flies, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_Flies>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("FlySpeed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 1000.0f)),
    PL_MEMBER_PROPERTY("PathLength", m_fPathLength)->AddAttributes(new plDefaultValueAttribute(0.2f), new plClampValueAttribute(0.0f, 100.0f)),
    PL_MEMBER_PROPERTY("MaxEmitterDistance", m_fMaxEmitterDistance)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 100.0f)),
    PL_MEMBER_PROPERTY("MaxSteeringAngle", m_MaxSteeringAngle)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(30)), new plClampValueAttribute(plAngle::MakeFromDegree(1.0f), plAngle::MakeFromDegree(180.0f))),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_Flies, 1, plRTTIDefaultAllocator<plParticleBehavior_Flies>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleBehaviorFactory_Flies::plParticleBehaviorFactory_Flies() = default;
plParticleBehaviorFactory_Flies::~plParticleBehaviorFactory_Flies() = default;

const plRTTI* plParticleBehaviorFactory_Flies::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_Flies>();
}

void plParticleBehaviorFactory_Flies::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_Flies* pBehavior = static_cast<plParticleBehavior_Flies*>(pObject);

  pBehavior->m_fSpeed = m_fSpeed;
  pBehavior->m_fPathLength = m_fPathLength;
  pBehavior->m_fMaxEmitterDistance = m_fMaxEmitterDistance;
  pBehavior->m_MaxSteeringAngle = m_MaxSteeringAngle;
}

void plParticleBehaviorFactory_Flies::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_ApplyVelocity>());
}

enum class BehaviorFliesVersion
{
  Version_0 = 0,
  Version_1,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleBehaviorFactory_Flies::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)BehaviorFliesVersion::Version_Current;
  inout_stream << uiVersion;

  inout_stream << m_fSpeed;
  inout_stream << m_fPathLength;
  inout_stream << m_fMaxEmitterDistance;
  inout_stream << m_MaxSteeringAngle;
}

void plParticleBehaviorFactory_Flies::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)BehaviorFliesVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_fSpeed;
  inout_stream >> m_fPathLength;
  inout_stream >> m_fMaxEmitterDistance;
  inout_stream >> m_MaxSteeringAngle;
}

void plParticleBehavior_Flies::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, false);

  m_TimeToChangeDir = plTime::MakeZero();
}

void plParticleBehavior_Flies::Process(plUInt64 uiNumElements)
{
  PL_PROFILE_SCOPE("PFX: Flies");

  const plTime tCur = GetOwnerEffect()->GetTotalEffectLifeTime();
  const bool bChangeDirection = tCur >= m_TimeToChangeDir;

  if (!bChangeDirection)
    return;

  m_TimeToChangeDir = tCur + plTime::MakeFromSeconds(m_fPathLength / m_fSpeed);

  const plVec3 vEmitterPos = GetOwnerSystem()->GetTransform().m_vPosition;
  const float fMaxDistanceToEmitterSquared = plMath::Square(m_fMaxEmitterDistance);

  plProcessingStreamIterator<plVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  plProcessingStreamIterator<plVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  plQuat qRot;

  while (!itPosition.HasReachedEnd())
  {
    // if (pLifeArray[i] == pMaxLifeArray[i])

    const plVec3 vPartToEm = vEmitterPos - itPosition.Current().GetAsVec3();
    const float fDist = vPartToEm.GetLengthSquared();
    const plVec3 vVelocity = itVelocity.Current();
    plVec3 vDir = vVelocity;
    vDir.NormalizeIfNotZero().IgnoreResult();

    if (fDist > fMaxDistanceToEmitterSquared)
    {
      plVec3 vPivot;
      vPivot = vDir.CrossRH(vPartToEm);
      vPivot.NormalizeIfNotZero().IgnoreResult();

      qRot = plQuat::MakeFromAxisAndAngle(vPivot, m_MaxSteeringAngle);

      itVelocity.Current() = qRot * vVelocity;
    }
    else
    {
      itVelocity.Current() = plVec3::MakeRandomDeviation(GetRNG(), m_MaxSteeringAngle, vDir) * m_fSpeed;
    }

    itPosition.Advance();
    itVelocity.Advance();
  }
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Flies);

