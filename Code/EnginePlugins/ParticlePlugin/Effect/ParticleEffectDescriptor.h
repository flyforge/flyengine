#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class PL_PARTICLEPLUGIN_DLL plParticleEffectDescriptor final : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEffectDescriptor, plReflectedClass);

public:
  plParticleEffectDescriptor();
  ~plParticleEffectDescriptor();

  void AddParticleSystem(plParticleSystemDescriptor* pSystem) { m_ParticleSystems.PushBack(pSystem); }
  void RemoveParticleSystem(plParticleSystemDescriptor* pSystem) { m_ParticleSystems.RemoveAndCopy(pSystem); }
  const plHybridArray<plParticleSystemDescriptor*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddEventReaction(plParticleEventReactionFactory* pSystem) { m_EventReactions.PushBack(pSystem); }
  void RemoveEventReaction(plParticleEventReactionFactory* pSystem) { m_EventReactions.RemoveAndCopy(pSystem); }
  const plHybridArray<plParticleEventReactionFactory*, 4>& GetEventReactions() const { return m_EventReactions; }


  void Save(plStreamWriter& inout_stream) const;
  void Load(plStreamReader& inout_stream);

  void ClearSystems();
  void ClearEventReactions();

  plEnum<plEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  bool m_bSimulateInLocalSpace = false;
  bool m_bAlwaysShared = false;
  float m_fApplyInstanceVelocity = 0.0f;
  plTime m_PreSimulateDuration;
  plMap<plString, float> m_FloatParameters;
  plMap<plString, plColor> m_ColorParameters;

private:
  plHybridArray<plParticleSystemDescriptor*, 4> m_ParticleSystems;
  plHybridArray<plParticleEventReactionFactory*, 4> m_EventReactions;
};
