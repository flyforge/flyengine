#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleBehaviorFactory_SizeCurve final : public plParticleBehaviorFactory
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehaviorFactory_SizeCurve, plParticleBehaviorFactory);

public:
  virtual const plRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(plParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(plStreamWriter& stream) const override;
  virtual void Load(plStreamReader& stream) override;

  void SetSizeCurveFile(const char* szFile);
  const char* GetSizeCurveFile() const;

  float m_fBaseSize;
  float m_fCurveScale;
  plCurve1DResourceHandle m_hCurve;
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleBehavior_SizeCurve final : public plParticleBehavior
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleBehavior_SizeCurve, plParticleBehavior);

public:
  float m_fBaseSize;
  float m_fCurveScale;
  plCurve1DResourceHandle m_hCurve;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements) override;
  virtual void Process(plUInt64 uiNumElements) override;

  plProcessingStream* m_pStreamLifeTime = nullptr;
  plProcessingStream* m_pStreamSize = nullptr;
  plUInt8 m_uiFirstToUpdate = 0;
  plUInt8 m_uiCurrentUpdateInterval = 8;
};
