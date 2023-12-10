#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Events/ParticleEventReaction.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReactionFactory, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("EventType", m_sEventType),
    PLASMA_MEMBER_PROPERTY("Probability", m_uiProbability)->AddAttributes(new plDefaultValueAttribute(100), new plClampValueAttribute(1, 100)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReaction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

plParticleEventReaction* plParticleEventReactionFactory::CreateEventReaction(plParticleEffectInstance* pOwner) const
{
  const plRTTI* pRtti = GetEventReactionType();

  plParticleEventReaction* pReaction = pRtti->GetAllocator()->Allocate<plParticleEventReaction>();
  pReaction->Reset(pOwner);
  pReaction->m_sEventName = plTempHashedString(m_sEventType.GetData());
  pReaction->m_uiProbability = m_uiProbability;

  CopyReactionProperties(pReaction, true);

  return pReaction;
}

enum class ReactionVersion
{
  Version_0 = 0,
  Version_1,
  Version_2, // added probability

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleEventReactionFactory::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)ReactionVersion::Version_Current;
  stream << uiVersion;

  // Version 1
  stream << m_sEventType;

  // Version 2
  stream << m_uiProbability;
}


void plParticleEventReactionFactory::Load(plStreamReader& stream)
{
  plUInt8 uiVersion = 0;
  stream >> uiVersion;

  PLASMA_ASSERT_DEV(uiVersion <= (int)ReactionVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  stream >> m_sEventType;

  if (uiVersion >= 2)
  {
    stream >> m_uiProbability;
  }
}

//////////////////////////////////////////////////////////////////////////

plParticleEventReaction::plParticleEventReaction() = default;
plParticleEventReaction::~plParticleEventReaction() = default;

void plParticleEventReaction::Reset(plParticleEffectInstance* pOwner)
{
  m_pOwnerEffect = pOwner;
}
