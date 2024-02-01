#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

plParticleEffectController::plParticleEffectController()
{
  m_hEffect.Invalidate();
}

plParticleEffectController::plParticleEffectController(const plParticleEffectController& rhs)
{
  m_pModule = rhs.m_pModule;
  m_hEffect = rhs.m_hEffect;
  m_pSharedInstanceOwner = rhs.m_pSharedInstanceOwner;
}

plParticleEffectController::plParticleEffectController(plParticleWorldModule* pModule, plParticleEffectHandle hEffect)
{
  m_pModule = pModule;
  m_hEffect = hEffect;
}

void plParticleEffectController::operator=(const plParticleEffectController& rhs)
{
  m_pModule = rhs.m_pModule;
  m_hEffect = rhs.m_hEffect;
  m_pSharedInstanceOwner = rhs.m_pSharedInstanceOwner;
}

plParticleEffectInstance* plParticleEffectController::GetInstance() const
{
  if (m_pModule == nullptr)
    return nullptr;

  plParticleEffectInstance* pEffect = nullptr;
  m_pModule->TryGetEffectInstance(m_hEffect, pEffect);
  return pEffect;
}

void plParticleEffectController::Create(const plParticleEffectResourceHandle& hEffectResource, plParticleWorldModule* pModule, plUInt64 uiRandomSeed,
  const char* szSharedName, const void* pSharedInstanceOwner, plArrayPtr<plParticleEffectFloatParam> floatParams,
  plArrayPtr<plParticleEffectColorParam> colorParams)
{
  m_pSharedInstanceOwner = pSharedInstanceOwner;

  // first get the new effect, to potentially increase a refcount to the same effect instance, before we decrease the refcount of our
  // current one
  plParticleEffectHandle hNewEffect;
  if (pModule != nullptr && hEffectResource.IsValid())
  {
    hNewEffect = pModule->CreateEffectInstance(hEffectResource, uiRandomSeed, szSharedName, m_pSharedInstanceOwner, floatParams, colorParams);
  }

  Invalidate();

  m_hEffect = hNewEffect;

  if (!m_hEffect.IsInvalidated())
    m_pModule = pModule;
}

bool plParticleEffectController::IsValid() const
{
  return (m_pModule != nullptr && !m_hEffect.IsInvalidated());
}

bool plParticleEffectController::IsAlive() const
{
  plParticleEffectInstance* pEffect = GetInstance();
  return pEffect != nullptr;
}

void plParticleEffectController::SetTransform(const plTransform& t, const plVec3& vParticleStartVelocity) const
{
  plParticleEffectInstance* pEffect = GetInstance();

  // shared effects are always simulated at the origin
  if (pEffect && m_pSharedInstanceOwner == nullptr)
  {
    pEffect->SetTransform(t, vParticleStartVelocity);
  }
}

void plParticleEffectController::Tick(const plTime& diff) const
{
  plParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->PreSimulate();
    pEffect->Update(diff);
  }
}

void plParticleEffectController::ExtractRenderData(plMsgExtractRenderData& ref_msg, const plTransform& systemTransform) const
{
  if (const plParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->SetIsVisible();

    m_pModule->ExtractEffectRenderData(pEffect, ref_msg, systemTransform);
  }
}

void plParticleEffectController::StopImmediate()
{
  if (m_pModule)
  {
    m_pModule->DestroyEffectInstance(m_hEffect, true, m_pSharedInstanceOwner);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

void plParticleEffectController::GetBoundingVolume(plBoundingBoxSphere& ref_volume) const
{
  if (plParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->GetBoundingVolume(ref_volume);
  }
}

void plParticleEffectController::UpdateWindSamples()
{
  if (plParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->UpdateWindSamples();
  }
}

void plParticleEffectController::ForceVisible()
{
  if (plParticleEffectInstance* pEffect = GetInstance())
  {
    pEffect->SetIsVisible();
  }
}

plUInt64 plParticleEffectController::GetNumActiveParticles() const
{
  if (plParticleEffectInstance* pEffect = GetInstance())
  {
    return pEffect->GetNumActiveParticles();
  }

  return 0;
}

void plParticleEffectController::SetParameter(const plTempHashedString& sName, float value)
{
  plParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->SetParameter(sName, value);
  }
}

void plParticleEffectController::SetParameter(const plTempHashedString& sName, const plColor& value)
{
  plParticleEffectInstance* pEffect = GetInstance();

  if (pEffect)
  {
    pEffect->SetParameter(sName, value);
  }
}

void plParticleEffectController::Invalidate()
{
  if (m_pModule)
  {
    m_pModule->DestroyEffectInstance(m_hEffect, false, m_pSharedInstanceOwner);

    m_pModule = nullptr;
    m_hEffect.Invalidate();
  }
}

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectController);
