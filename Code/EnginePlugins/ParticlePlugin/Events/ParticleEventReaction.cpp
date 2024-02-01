#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Events/ParticleEventReaction.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReactionFactory, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("EventType", m_sEventType),
    PL_MEMBER_PROPERTY("Probability", m_uiProbability)->AddAttributes(new plDefaultValueAttribute(100), new plClampValueAttribute(1, 100)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEventReaction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

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

void plParticleEventReactionFactory::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)ReactionVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEventType;

  // Version 2
  inout_stream << m_uiProbability;
}


void plParticleEventReactionFactory::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)ReactionVersion::Version_Current, "Invalid version {0}", uiVersion);

  // Version 1
  inout_stream >> m_sEventType;

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiProbability;
  }
}

//////////////////////////////////////////////////////////////////////////

plParticleEventReaction::plParticleEventReaction() = default;
plParticleEventReaction::~plParticleEventReaction() = default;

void plParticleEventReaction::Reset(plParticleEffectInstance* pOwner)
{
  m_pOwnerEffect = pOwner;
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Events_ParticleEventReaction);

