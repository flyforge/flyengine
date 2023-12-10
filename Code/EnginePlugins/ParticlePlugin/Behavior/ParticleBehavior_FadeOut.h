#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_FadeOut final : public plParticleBehaviorFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_FadeOut, plParticleBehaviorFactory);

public:
  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  float m_fStartAlpha = 1.0f;
  float m_fExponent = 1.0f;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleBehavior_FadeOut final : public plParticleBehavior
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehavior_FadeOut, plParticleBehavior);

public:
  float m_fStartAlpha = 1.0f;
  float m_fExponent = 1.0f;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamLifeTime = nullptr;
  plProcessingStream* m_pStreamColor = nullptr;
  plUInt8 m_uiFirstToUpdate = 0;
  plUInt8 m_uiCurrentUpdateInterval = 2;
};
