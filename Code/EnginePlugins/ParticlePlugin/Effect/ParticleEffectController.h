#pragma once

#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class PLASMA_PARTICLEPLUGIN_DLL plParticleEffectController
{
public:
  plParticleEffectController();
  plParticleEffectController(const plParticleEffectController& rhs);
  void operator=(const plParticleEffectController& rhs);

  void Create(const plParticleEffectResourceHandle& hEffectResource, plParticleWorldModule* pModule, plUInt64 uiRandomSeed,
    const char* szSharedName /*= nullptr*/, const void* pSharedInstanceOwner /*= nullptr*/, plArrayPtr<plParticleEffectFloatParam> floatParams,
    plArrayPtr<plParticleEffectColorParam> colorParams);

  bool IsValid() const;
  void Invalidate();

  bool IsAlive() const;
  bool IsSharedInstance() const { return m_pSharedInstanceOwner != nullptr; }

  bool IsContinuousEffect() const { return GetInstance()->IsContinuous(); }

  void SetTransform(const plTransform& t, const plVec3& vParticleStartVelocity) const;

  void Tick(const plTime& tDiff) const;

  void ExtractRenderData(plMsgExtractRenderData& msg, const plTransform& systemTransform) const;

  void StopImmediate();

  /// \brief Returns the bounding volume of the effect.
  /// The volume is in the local space of the effect.
  void GetBoundingVolume(plBoundingBoxSphere& volume) const;

  void UpdateWindSamples();

  /// \brief Ensures that the effect is considered to be 'visible', which affects the update rate.
  void ForceVisible();

  plUInt64 GetNumActiveParticles() const;

  /// \name Effect Parameters
  ///@{
public:
  /// \brief Passes an effect parameter on to the effect instance
  void SetParameter(const plTempHashedString& name, float value);

  /// \brief Passes an effect parameter on to the effect instance
  void SetParameter(const plTempHashedString& name, const plColor& value);

  ///@}

private:
  friend class plParticleWorldModule;

  plParticleEffectController(plParticleWorldModule* pModule, plParticleEffectHandle hEffect);
  plParticleEffectInstance* GetInstance() const;

  const void* m_pSharedInstanceOwner = nullptr;
  plParticleWorldModule* m_pModule = nullptr;
  plParticleEffectHandle m_hEffect;
};
