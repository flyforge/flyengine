#pragma once

#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using plParticleEffectResourceHandle = plTypedResourceHandle<class plParticleEffectResource>;

class PL_PARTICLEPLUGIN_DLL plParticleTypeEffectFactory final : public plParticleTypeFactory
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTypeEffectFactory, plParticleTypeFactory);

public:
  plParticleTypeEffectFactory();
  ~plParticleTypeEffectFactory();

  virtual const plRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(plParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& inout_stream) const override;
  virtual void Load(plStreamReader& inout_stream) override;

  plString m_sEffect;
  plString m_sSharedInstanceName; // to be removed
};

class PL_PARTICLEPLUGIN_DLL plParticleTypeEffect final : public plParticleType
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleTypeEffect, plParticleType);

public:
  plParticleTypeEffect();
  ~plParticleTypeEffect();

  plParticleEffectResourceHandle m_hEffect;
  // plString m_sSharedInstanceName;

  virtual void CreateRequiredStreams() override;
  virtual void ExtractTypeRenderData(plMsgExtractRenderData& ref_msg, const plTransform& instanceTransform) const override;

  virtual float GetMaxParticleRadius(float fParticleSize) const override { return m_fMaxEffectRadius; }

protected:
  friend class plParticleTypeEffectFactory;

  virtual void OnReset() override;
  virtual void Process(plUInt64 uiNumElements) override;
  void OnParticleDeath(const plStreamGroupElementRemovedEvent& e);
  void ClearEffects(bool bInterruptImmediately);

  float m_fMaxEffectRadius = 1.0f;
  plProcessingStream* m_pStreamPosition = nullptr;
  plProcessingStream* m_pStreamEffectID = nullptr;
};
