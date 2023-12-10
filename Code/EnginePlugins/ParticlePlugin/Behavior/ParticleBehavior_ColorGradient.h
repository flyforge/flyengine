#pragma once

#include <Core/Curves/ColorGradientResource.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_ColorGradient final : public plParticleBehaviorFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_ColorGradient, plParticleBehaviorFactory);

public:
  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  // ************************************* PROPERTIES ***********************************

  void SetColorGradient(const plColorGradientResourceHandle& hResource) { m_hGradient = hResource; }
  PLASMA_ALWAYS_INLINE const plColorGradientResourceHandle& GetColorGradient() const { return m_hGradient; }

  void SetColorGradientFile(const char* szFile);
  const char* GetColorGradientFile() const;

  plEnum<plParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  plColor m_TintColor = plColor::White;

private:
  plColorGradientResourceHandle m_hGradient;
};


class PLASMA_PARTICLEPLUGIN_DLL plParticleBehavior_ColorGradient final : public plParticleBehavior
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehavior_ColorGradient, plParticleBehavior);

public:
  plColorGradientResourceHandle m_hGradient;
  plEnum<plParticleColorGradientMode> m_GradientMode;
  float m_fMaxSpeed = 1.0f;
  plColor m_TintColor;

  virtual void CreateRequiredStreams() override;

protected:
  friend class plParticleBehaviorFactory_ColorGradient;

  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamLifeTime = nullptr;
  plProcessingStream* m_pStreamColor = nullptr;
  plProcessingStream* m_pStreamVelocity = nullptr;
  plColor m_InitColor;
  plUInt8 m_uiFirstToUpdate = 0;
  plUInt8 m_uiCurrentUpdateInterval = 8;
};
