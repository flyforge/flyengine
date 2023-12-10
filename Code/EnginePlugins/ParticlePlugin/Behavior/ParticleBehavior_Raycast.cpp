#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_Raycast.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_LastPosition.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehaviorFactory_Raycast, 1, plRTTIDefaultAllocator<plParticleBehaviorFactory_Raycast>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Reaction", plParticleRaycastHitReaction, m_Reaction),
    PLASMA_MEMBER_PROPERTY("BounceFactor", m_fBounceFactor)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_MEMBER_PROPERTY("OnCollideEvent", m_sOnCollideEvent),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleBehavior_Raycast, 1, plRTTIDefaultAllocator<plParticleBehavior_Raycast>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plParticleRaycastHitReaction, 1)
  PLASMA_ENUM_CONSTANTS(plParticleRaycastHitReaction::Bounce, plParticleRaycastHitReaction::Die, plParticleRaycastHitReaction::Stop)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

plParticleBehaviorFactory_Raycast::plParticleBehaviorFactory_Raycast() = default;
plParticleBehaviorFactory_Raycast::~plParticleBehaviorFactory_Raycast() = default;

const plRTTI* plParticleBehaviorFactory_Raycast::GetBehaviorType() const
{
  return plGetStaticRTTI<plParticleBehavior_Raycast>();
}

void plParticleBehaviorFactory_Raycast::CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const
{
  plParticleBehavior_Raycast* pBehavior = static_cast<plParticleBehavior_Raycast*>(pObject);

  pBehavior->m_Reaction = m_Reaction;
  pBehavior->m_uiCollisionLayer = m_uiCollisionLayer;
  pBehavior->m_sOnCollideEvent = plTempHashedString(m_sOnCollideEvent.GetData());
  pBehavior->m_fBounceFactor = m_fBounceFactor;

  pBehavior->m_pPhysicsModule = (plPhysicsWorldModuleInterface*)pBehavior->GetOwnerSystem()->GetOwnerWorldModule()->GetCachedWorldModule(plGetStaticRTTI<plPhysicsWorldModuleInterface>());
}

enum class BehaviorRaycastVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added event
  Version_3, // added bounce factor

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void plParticleBehaviorFactory_Raycast::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)BehaviorRaycastVersion::Version_Current;
  stream << uiVersion;

  stream << m_uiCollisionLayer;
  stream << m_sOnCollideEvent;

  plParticleRaycastHitReaction::StorageType hr = m_Reaction.GetValue();
  stream << hr;

  stream << m_fBounceFactor;
}

void plParticleBehaviorFactory_Raycast::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)BehaviorRaycastVersion::Version_Current, "Invalid version {0}", uiVersion);

  if (uiVersion >= 2)
  {
    stream >> m_uiCollisionLayer;
    stream >> m_sOnCollideEvent;

    plParticleRaycastHitReaction::StorageType hr;
    stream >> hr;
    m_Reaction.SetValue(hr);
  }

  if (uiVersion >= 3)
  {
    stream >> m_fBounceFactor;
  }
}

void plParticleBehaviorFactory_Raycast::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_FinalizerDeps) const
{
  inout_FinalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_ApplyVelocity>());
  inout_FinalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_LastPosition>());
}

//////////////////////////////////////////////////////////////////////////

plParticleBehavior_Raycast::plParticleBehavior_Raycast()
{
  // do this right after plParticleFinalizer_ApplyVelocity has run
  m_fPriority = 526.0f;
}

void plParticleBehavior_Raycast::CreateRequiredStreams()
{
  CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
  CreateStream("LastPosition", plProcessingStream::DataType::Float3, &m_pStreamLastPosition, false);
  CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
}

void plParticleBehavior_Raycast::Process(plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Raycast");

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  plProcessingStreamIterator<plVec4> itPosition(m_pStreamPosition, uiNumElements, 0);
  plProcessingStreamIterator<const plVec3> itLastPosition(m_pStreamLastPosition, uiNumElements, 0);
  plProcessingStreamIterator<plVec3> itVelocity(m_pStreamVelocity, uiNumElements, 0);

  plPhysicsCastResult hitResult;

  plUInt32 i = 0;
  while (!itPosition.HasReachedEnd())
  {
    const plVec3 vLastPos = itLastPosition.Current();
    const plVec3 vCurPos = itPosition.Current().GetAsVec3();

    if (!vLastPos.IsZero())
    {
      const plVec3 vChange = vCurPos - vLastPos;

      if (!vChange.IsZero(0.001f))
      {
        plVec3 vDirection = vChange;

        const float fMaxLen = vDirection.GetLengthAndNormalize();

        plPhysicsQueryParameters query(m_uiCollisionLayer);
        query.m_ShapeTypes = plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic;

        if (m_pPhysicsModule != nullptr && m_pPhysicsModule->Raycast(hitResult, vLastPos, vDirection, fMaxLen, query))
        {
          if (m_Reaction == plParticleRaycastHitReaction::Bounce)
          {
            const plVec3 vNewDir = vChange.GetReflectedVector(hitResult.m_vNormal) * m_fBounceFactor;

            if (vNewDir.GetLengthSquared() < plMath::Square(0.01f))
            {
              itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
              itVelocity.Current().SetZero();
            }
            else
            {
              itPosition.Current() = plVec3(hitResult.m_vPosition + vNewDir).GetAsVec4(0);
              itVelocity.Current() = vNewDir / tDiff;
            }
          }
          else if (m_Reaction == plParticleRaycastHitReaction::Die)
          {
            /// \todo Get current element index from iterator ?
            m_pStreamGroup->RemoveElement(i);
          }
          else if (m_Reaction == plParticleRaycastHitReaction::Stop)
          {
            itPosition.Current() = hitResult.m_vPosition.GetAsPositionVec4();
            itVelocity.Current().SetZero();
          }

          if (!m_sOnCollideEvent.IsEmpty())
          {
            plParticleEvent e;
            e.m_EventType = m_sOnCollideEvent;
            e.m_vPosition = hitResult.m_vPosition;
            e.m_vNormal = hitResult.m_vNormal;
            e.m_vDirection = vDirection;

            GetOwnerEffect()->AddParticleEvent(e);
          }
        }
      }
    }

    itPosition.Advance();
    itLastPosition.Advance();
    itVelocity.Advance();

    ++i;
  }
}

void plParticleBehavior_Raycast::RequestRequiredWorldModulesForCache(plParticleWorldModule* pParticleModule)
{
  pParticleModule->CacheWorldModule<plPhysicsWorldModuleInterface>();
}

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Behavior_ParticleBehavior_Raycast);
