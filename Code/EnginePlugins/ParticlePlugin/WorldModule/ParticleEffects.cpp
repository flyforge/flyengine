#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plParticleEffectHandle plParticleWorldModule::InternalCreateEffectInstance(const plParticleEffectResourceHandle& hResource, plUInt64 uiRandomSeed,
  bool bIsShared, plArrayPtr<plParticleEffectFloatParam> floatParams, plArrayPtr<plParticleEffectColorParam> colorParams)
{
  PL_LOCK(m_Mutex);

  plParticleEffectInstance* pInstance = nullptr;

  if (!m_ParticleEffectsFreeList.IsEmpty())
  {
    pInstance = m_ParticleEffectsFreeList.PeekBack();
    m_ParticleEffectsFreeList.PopBack();
  }
  else
  {
    pInstance = &m_ParticleEffects.ExpandAndGetRef();
  }

  plParticleEffectHandle hEffectHandle(m_ActiveEffects.Insert(pInstance));
  pInstance->Construct(hEffectHandle, hResource, GetWorld(), this, uiRandomSeed, bIsShared, floatParams, colorParams);

  return hEffectHandle;
}

plParticleEffectHandle plParticleWorldModule::InternalCreateSharedEffectInstance(
  const char* szSharedName, const plParticleEffectResourceHandle& hResource, plUInt64 uiRandomSeed, const void* pSharedInstanceOwner)
{
  PL_LOCK(m_Mutex);

  plStringBuilder fullName;
  fullName.SetFormat("{{0}}-{{1}}[{2}]", szSharedName, hResource.GetResourceID(), uiRandomSeed);

  bool bExisted = false;
  auto it = m_SharedEffects.FindOrAdd(fullName, &bExisted);
  plParticleEffectInstance* pEffect = nullptr;

  if (bExisted)
  {
    TryGetEffectInstance(it.Value(), pEffect);
  }

  if (!pEffect)
  {
    it.Value() =
      InternalCreateEffectInstance(hResource, uiRandomSeed, true, plArrayPtr<plParticleEffectFloatParam>(), plArrayPtr<plParticleEffectColorParam>());
    TryGetEffectInstance(it.Value(), pEffect);
  }

  PL_ASSERT_DEBUG(pEffect != nullptr, "Invalid effect pointer");
  pEffect->AddSharedInstance(pSharedInstanceOwner);

  return it.Value();
}


plParticleEffectHandle plParticleWorldModule::CreateEffectInstance(const plParticleEffectResourceHandle& hResource, plUInt64 uiRandomSeed,
  const char* szSharedName, const void*& inout_pSharedInstanceOwner, plArrayPtr<plParticleEffectFloatParam> floatParams,
  plArrayPtr<plParticleEffectColorParam> colorParams)
{
  PL_ASSERT_DEBUG(hResource.IsValid(), "Invalid Particle Effect resource handle");

  bool bIsShared = !plStringUtils::IsNullOrEmpty(szSharedName) && (inout_pSharedInstanceOwner != nullptr);

  if (!bIsShared)
  {
    plResourceLock<plParticleEffectResource> pResource(hResource, plResourceAcquireMode::BlockTillLoaded);
    bIsShared |= pResource->GetDescriptor().m_Effect.m_bAlwaysShared;
  }

  if (!bIsShared)
  {
    inout_pSharedInstanceOwner = nullptr;
    return InternalCreateEffectInstance(hResource, uiRandomSeed, false, floatParams, colorParams);
  }
  else
  {
    return InternalCreateSharedEffectInstance(szSharedName, hResource, uiRandomSeed, inout_pSharedInstanceOwner);
  }
}

void plParticleWorldModule::DestroyEffectInstance(const plParticleEffectHandle& hEffect, bool bInterruptImmediately, const void* pSharedInstanceOwner)
{
  PL_LOCK(m_Mutex);

  plParticleEffectInstance* pInstance = nullptr;
  if (TryGetEffectInstance(hEffect, pInstance))
  {
    if (pSharedInstanceOwner != nullptr)
    {
      pInstance->RemoveSharedInstance(pSharedInstanceOwner);
      return; // never delete these
    }

    if (!pInstance->m_bIsFinishing)
    {
      // make sure not to insert it into m_FinishingEffects twice
      pInstance->m_bIsFinishing = true;
      pInstance->SetEmitterEnabled(false);
      m_FinishingEffects.PushBack(pInstance);

      if (!bInterruptImmediately)
      {
        m_NeedFinisherComponent.PushBack(pInstance);
      }
    }

    if (bInterruptImmediately)
    {
      pInstance->Interrupt();
    }
  }
}

bool plParticleWorldModule::TryGetEffectInstance(const plParticleEffectHandle& hEffect, plParticleEffectInstance*& out_pEffect)
{
  return m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), out_pEffect);
}

bool plParticleWorldModule::TryGetEffectInstance(const plParticleEffectHandle& hEffect, const plParticleEffectInstance*& out_pEffect) const
{
  plParticleEffectInstance* pEffect = nullptr;
  bool bResult = m_ActiveEffects.TryGetValue(hEffect.GetInternalID(), pEffect);
  out_pEffect = pEffect;
  return bResult;
}

void plParticleWorldModule::UpdateEffects(const plWorldModule::UpdateContext& context)
{
  PL_LOCK(m_Mutex);

  DestroyFinishedEffects();
  ReconfigureEffects();

  m_EffectUpdateTaskGroup = plTaskSystem::CreateTaskGroup(plTaskPriority::LateThisFrame);

  const plTime tDiff = GetWorld()->GetClock().GetTimeDiff();
  for (plUInt32 i = 0; i < m_ParticleEffects.GetCount(); ++i)
  {
    if (!m_ParticleEffects[i].ShouldBeUpdated())
      continue;

    m_ParticleEffects[i].ProcessEventQueues();

    const plSharedPtr<plTask>& pTask = m_ParticleEffects[i].GetUpdateTask();
    static_cast<plParticleEffectUpdateTask*>(pTask.Borrow())->m_UpdateDiff = tDiff;

    plTaskSystem::AddTaskToGroup(m_EffectUpdateTaskGroup, pTask);
  }

  plTaskSystem::StartTaskGroup(m_EffectUpdateTaskGroup);
}

void plParticleWorldModule::DestroyFinishedEffects()
{
  PL_LOCK(m_Mutex);

  for (plUInt32 i = 0; i < m_FinishingEffects.GetCount();)
  {
    plParticleEffectInstance* pEffect = m_FinishingEffects[i];

    if (!pEffect->HasActiveParticles())
    {
      if (m_ActiveEffects.Remove(pEffect->GetHandle().GetInternalID()))
      {
        pEffect->Destruct();

        m_ParticleEffectsFreeList.PushBack(pEffect);
      }

      m_FinishingEffects.RemoveAtAndSwap(i);
    }
    else
    {
      ++i;
    }
  }

  for (plUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
  {
    plParticleEffectInstance* pEffect = m_NeedFinisherComponent[i];

    CreateFinisherComponent(pEffect);
  }

  m_NeedFinisherComponent.Clear();
}

void plParticleWorldModule::ReconfigureEffects()
{
  PL_LOCK(m_Mutex);

  for (auto pEffect : m_EffectsToReconfigure)
  {
    pEffect->Reconfigure(false, plArrayPtr<plParticleEffectFloatParam>(), plArrayPtr<plParticleEffectColorParam>());
  }

  m_EffectsToReconfigure.Clear();
}



PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleEffects);
