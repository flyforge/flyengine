#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEffectDescriptor, 2, plRTTIDefaultAllocator<plParticleEffectDescriptor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("WhenInvisible", plEffectInvisibleUpdateRate, m_InvisibleUpdateRate),
    PLASMA_MEMBER_PROPERTY("AlwaysShared", m_bAlwaysShared),
    PLASMA_MEMBER_PROPERTY("SimulateInLocalSpace", m_bSimulateInLocalSpace),
    PLASMA_MEMBER_PROPERTY("ApplyOwnerVelocity", m_fApplyInstanceVelocity)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_MEMBER_PROPERTY("PreSimulateDuration", m_PreSimulateDuration),
    PLASMA_MAP_MEMBER_PROPERTY("FloatParameters", m_FloatParameters),
    PLASMA_MAP_MEMBER_PROPERTY("ColorParameters", m_ColorParameters)->AddAttributes(new plExposeColorAlphaAttribute),
    PLASMA_SET_ACCESSOR_PROPERTY("ParticleSystems", GetParticleSystems, AddParticleSystem, RemoveParticleSystem)->AddFlags(plPropertyFlags::PointerOwner),
    PLASMA_SET_ACCESSOR_PROPERTY("EventReactions", GetEventReactions, AddEventReaction, RemoveEventReaction)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEffectDescriptor::plParticleEffectDescriptor() {}

plParticleEffectDescriptor::~plParticleEffectDescriptor()
{
  ClearSystems();
  ClearEventReactions();
}

void plParticleEffectDescriptor::ClearSystems()
{
  for (auto pSystem : m_ParticleSystems)
  {
    pSystem->GetDynamicRTTI()->GetAllocator()->Deallocate(pSystem);
  }

  m_ParticleSystems.Clear();
}


void plParticleEffectDescriptor::ClearEventReactions()
{
  for (auto pReaction : m_EventReactions)
  {
    pReaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pReaction);
  }

  m_EventReactions.Clear();
}

enum class ParticleEffectVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4,
  Version_5, // m_bAlwaysShared
  Version_6, // added parameters
  Version_7, // added instance velocity
  Version_8, // added event reactions
  Version_9, // breaking change

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};

void plParticleEffectDescriptor::Save(plStreamWriter& stream) const
{
  const plUInt8 uiVersion = (int)ParticleEffectVersion::Version_Current;

  stream << uiVersion;

  const plUInt32 uiNumSystems = m_ParticleSystems.GetCount();

  stream << uiNumSystems;

  // Version 3
  stream << m_bSimulateInLocalSpace;
  stream << m_PreSimulateDuration;
  // Version 4
  stream << m_InvisibleUpdateRate;
  // Version 5
  stream << m_bAlwaysShared;

  // Version 3
  for (auto pSystem : m_ParticleSystems)
  {
    stream << pSystem->GetDynamicRTTI()->GetTypeName();

    pSystem->Save(stream);
  }

  // Version 6
  {
    plUInt8 paramCol = static_cast<plUInt8>(m_ColorParameters.GetCount());
    stream << paramCol;
    for (auto it = m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      stream << it.Key();
      stream << it.Value();
    }

    plUInt8 paramFloat = static_cast<plUInt8>(m_FloatParameters.GetCount());
    stream << paramFloat;
    for (auto it = m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      stream << it.Key();
      stream << it.Value();
    }
  }

  // Version 7
  stream << m_fApplyInstanceVelocity;

  // Version 8
  {
    const plUInt32 uiNumReactions = m_EventReactions.GetCount();
    stream << uiNumReactions;

    for (auto pReaction : m_EventReactions)
    {
      stream << pReaction->GetDynamicRTTI()->GetTypeName();

      pReaction->Save(stream);
    }
  }
}


void plParticleEffectDescriptor::Load(plStreamReader& stream)
{
  ClearSystems();
  ClearEventReactions();

  plUInt8 uiVersion = 0;
  stream >> uiVersion;
  PLASMA_ASSERT_DEV(uiVersion <= (int)ParticleEffectVersion::Version_Current, "Unknown particle effect template version {0}", uiVersion);

  if (uiVersion < (int)ParticleEffectVersion::Version_9)
  {
    plLog::SeriousWarning("Unsupported old particle effect version");
    return;
  }

  plUInt32 uiNumSystems = 0;
  stream >> uiNumSystems;

  stream >> m_bSimulateInLocalSpace;
  stream >> m_PreSimulateDuration;
  stream >> m_InvisibleUpdateRate;
  stream >> m_bAlwaysShared;

  m_ParticleSystems.SetCountUninitialized(uiNumSystems);

  plStringBuilder sType;

  for (auto& pSystem : m_ParticleSystems)
  {
    stream >> sType;

    const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
    PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect type '{0}'", sType);

    pSystem = pRtti->GetAllocator()->Allocate<plParticleSystemDescriptor>();

    pSystem->Load(stream);
  }

  plStringBuilder key;
  m_ColorParameters.Clear();
  m_FloatParameters.Clear();

  plUInt8 paramCol;
  stream >> paramCol;
  for (plUInt32 i = 0; i < paramCol; ++i)
  {
    plColor val;
    stream >> key;
    stream >> val;
    m_ColorParameters[key] = val;
  }

  plUInt8 paramFloat;
  stream >> paramFloat;
  for (plUInt32 i = 0; i < paramFloat; ++i)
  {
    float val;
    stream >> key;
    stream >> val;
    m_FloatParameters[key] = val;
  }

  stream >> m_fApplyInstanceVelocity;

  plUInt32 uiNumReactions = 0;
  stream >> uiNumReactions;

  m_EventReactions.SetCountUninitialized(uiNumReactions);

  for (auto& pReaction : m_EventReactions)
  {
    stream >> sType;

    const plRTTI* pRtti = plRTTI::FindTypeByName(sType);
    PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Unknown particle effect event reaction type '{0}'", sType);

    pReaction = pRtti->GetAllocator()->Allocate<plParticleEventReactionFactory>();

    pReaction->Load(stream);
  }
}

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectDescriptor);
