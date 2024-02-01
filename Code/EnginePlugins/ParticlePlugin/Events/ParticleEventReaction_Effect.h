#pragma once

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>

class PL_PARTICLEPLUGIN_DLL plParticleEventReactionFactory_Effect final : public plParticleEventReactionFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEventReactionFactory_Effect, plParticleEventReactionFactory);

public:
  plParticleEventReactionFactory_Effect();

  virtual const plRTTI* GetEventReactionType() const override;
  virtual void CopyReactionProperties(plParticleEventReaction* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  plString m_sEffect;
  plEnum<plSurfaceInteractionAlignment> m_Alignment;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  const plRangeView<const char*, plUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const plVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, plVariant& out_value) const;

private:
  plSharedPtr<plParticleEffectParameters> m_pParameters;
};

class PL_PARTICLEPLUGIN_DLL plParticleEventReaction_Effect final : public plParticleEventReaction
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEventReaction_Effect, plParticleEventReaction);

public:
  plParticleEventReaction_Effect();
  ~plParticleEventReaction_Effect();

  plParticleEffectResourceHandle m_hEffect;
  plEnum<plSurfaceInteractionAlignment> m_Alignment;
  plSharedPtr<plParticleEffectParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const plParticleEvent& e) override;
};
