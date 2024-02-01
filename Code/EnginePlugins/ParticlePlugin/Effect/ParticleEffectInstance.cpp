#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Effect/ParticleEffectDescriptor.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

plParticleEffectInstance::plParticleEffectInstance()
{
  m_pTask = PL_DEFAULT_NEW(plParticleEffectUpdateTask, this);
  m_pTask->ConfigureTask("Particle Effect Update", plTaskNesting::Maybe);

  m_pOwnerModule = nullptr;

  Destruct();
}

plParticleEffectInstance::~plParticleEffectInstance()
{
  Destruct();
}

void plParticleEffectInstance::Construct(plParticleEffectHandle hEffectHandle, const plParticleEffectResourceHandle& hResource, plWorld* pWorld, plParticleWorldModule* pOwnerModule, plUInt64 uiRandomSeed, bool bIsShared, plArrayPtr<plParticleEffectFloatParam> floatParams, plArrayPtr<plParticleEffectColorParam> colorParams)
{
  m_hEffectHandle = hEffectHandle;
  m_pWorld = pWorld;
  m_pOwnerModule = pOwnerModule;
  m_hResource = hResource;
  m_bIsSharedEffect = bIsShared;
  m_bEmitterEnabled = true;
  m_bIsFinishing = false;
  m_BoundingVolume = plBoundingBoxSphere::MakeInvalid();
  m_ElapsedTimeSinceUpdate = plTime::MakeZero();
  m_EffectIsVisible = plTime::MakeZero();
  m_iMinSimStepsToDo = 4;
  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_TotalEffectLifeTime = plTime::MakeZero();
  m_pVisibleIf = nullptr;
  m_uiRandomSeed = uiRandomSeed;

  if (uiRandomSeed == 0)
    m_Random.InitializeFromCurrentTime();
  else
    m_Random.Initialize(uiRandomSeed);

  Reconfigure(true, floatParams, colorParams);
}

void plParticleEffectInstance::Destruct()
{
  Interrupt();

  m_SharedInstances.Clear();
  m_hEffectHandle.Invalidate();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_bIsSharedEffect = false;
  m_pWorld = nullptr;
  m_hResource.Invalidate();
  m_hEffectHandle.Invalidate();
  m_uiReviveTimeout = 5;
}

void plParticleEffectInstance::Interrupt()
{
  ClearParticleSystems();
  ClearEventReactions();
  m_bEmitterEnabled = false;
}

void plParticleEffectInstance::SetEmitterEnabled(bool bEnable)
{
  m_bEmitterEnabled = bEnable;

  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    }
  }
}


bool plParticleEffectInstance::HasActiveParticles() const
{
  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->HasActiveParticles())
        return true;
    }
  }

  return false;
}


void plParticleEffectInstance::ClearParticleSystem(plUInt32 index)
{
  if (m_ParticleSystems[index])
  {
    m_pOwnerModule->DestroySystemInstance(m_ParticleSystems[index]);
    m_ParticleSystems[index] = nullptr;
  }
}

void plParticleEffectInstance::ClearParticleSystems()
{
  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    ClearParticleSystem(i);
  }

  m_ParticleSystems.Clear();
}


void plParticleEffectInstance::ClearEventReactions()
{
  for (plUInt32 i = 0; i < m_EventReactions.GetCount(); ++i)
  {
    if (m_EventReactions[i])
    {
      m_EventReactions[i]->GetDynamicRTTI()->GetAllocator()->Deallocate(m_EventReactions[i]);
    }
  }

  m_EventReactions.Clear();
}

bool plParticleEffectInstance::IsContinuous() const
{
  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      if (m_ParticleSystems[i]->IsContinuous())
        return true;
    }
  }

  return false;
}

void plParticleEffectInstance::PreSimulate()
{
  if (m_PreSimulateDuration.GetSeconds() == 0.0)
    return;

  PassTransformToSystems();

  // Pre-simulate the effect, if desired, to get it into a 'good looking' state

  // simulate in large steps to get close
  {
    const plTime tDiff = plTime::MakeFromSeconds(0.5);
    while (m_PreSimulateDuration.GetSeconds() > 10.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // finer steps
  {
    const plTime tDiff = plTime::MakeFromSeconds(0.2);
    while (m_PreSimulateDuration.GetSeconds() > 5.0)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // even finer
  {
    const plTime tDiff = plTime::MakeFromSeconds(0.1);
    while (m_PreSimulateDuration.GetSeconds() >= 0.1)
    {
      StepSimulation(tDiff);
      m_PreSimulateDuration -= tDiff;
    }
  }

  // final step if necessary
  if (m_PreSimulateDuration.GetSeconds() > 0.0)
  {
    StepSimulation(m_PreSimulateDuration);
    m_PreSimulateDuration = plTime::MakeFromSeconds(0);
  }

  if (!IsContinuous())
  {
    // Can't check this at the beginning, because the particle systems are only set up during StepSimulation.
    plLog::Warning("Particle pre-simulation is enabled on an effect that is not continuous.");
  }
}

void plParticleEffectInstance::SetIsVisible() const
{
  // if it is visible this frame, also render it the next few frames
  // this has multiple purposes:
  // 1) it fixes the transition when handing off an effect from a
  //    plParticleComponent to a plParticleFinisherComponent
  //    though this would only need one frame overlap
  // 2) The bounding volume for culling is only computed every couple of frames
  //    so it may be too small and culling could be imprecise
  //    by just rendering it the next 100ms, no matter what, the bounding volume
  //    does not need to be updated so frequently
  m_EffectIsVisible = plClock::GetGlobalClock()->GetAccumulatedTime() + plTime::MakeFromSeconds(0.1);
}


void plParticleEffectInstance::SetVisibleIf(plParticleEffectInstance* pOtherVisible)
{
  PL_ASSERT_DEV(pOtherVisible != this, "Invalid effect");
  m_pVisibleIf = pOtherVisible;
}

bool plParticleEffectInstance::IsVisible() const
{
  if (m_pVisibleIf != nullptr)
  {
    return m_pVisibleIf->IsVisible();
  }

  return m_EffectIsVisible >= plClock::GetGlobalClock()->GetAccumulatedTime();
}

void plParticleEffectInstance::Reconfigure(bool bFirstTime, plArrayPtr<plParticleEffectFloatParam> floatParams, plArrayPtr<plParticleEffectColorParam> colorParams)
{
  if (!m_hResource.IsValid())
  {
    plLog::Error("Effect Reconfigure: Effect Resource is invalid");
    return;
  }

  plResourceLock<plParticleEffectResource> pResource(m_hResource, plResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pResource->GetDescriptor().m_Effect;
  const auto& systems = desc.GetParticleSystems();

  m_Transform.SetIdentity();
  m_TransformForNextFrame.SetIdentity();
  m_vVelocity.SetZero();
  m_vVelocityForNextFrame.SetZero();
  m_fApplyInstanceVelocity = desc.m_fApplyInstanceVelocity;
  m_bSimulateInLocalSpace = desc.m_bSimulateInLocalSpace;
  m_InvisibleUpdateRate = desc.m_InvisibleUpdateRate;

  // parameters
  {
    m_FloatParameters.Clear();
    m_ColorParameters.Clear();

    for (auto it = desc.m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(plTempHashedString(it.Key().GetData()), it.Value());
    }

    for (auto it = desc.m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      SetParameter(plTempHashedString(it.Key().GetData()), it.Value());
    }

    // shared effects do not support per-instance parameters
    if (m_bIsSharedEffect)
    {
      if (!floatParams.IsEmpty() || !colorParams.IsEmpty())
      {
        plLog::Warning("Shared particle effects do not support effect parameters");
      }
    }
    else
    {
      for (plUInt32 p = 0; p < floatParams.GetCount(); ++p)
      {
        SetParameter(floatParams[p].m_sName, floatParams[p].m_Value);
      }

      for (plUInt32 p = 0; p < colorParams.GetCount(); ++p)
      {
        SetParameter(colorParams[p].m_sName, colorParams[p].m_Value);
      }
    }
  }

  if (bFirstTime)
  {
    m_PreSimulateDuration = desc.m_PreSimulateDuration;
  }

  // TODO Check max number of particles etc. to reset

  if (m_ParticleSystems.GetCount() != systems.GetCount())
  {
    // reset everything
    ClearParticleSystems();
  }

  m_ParticleSystems.SetCount(systems.GetCount());

  struct MulCount
  {
    PL_DECLARE_POD_TYPE();

    float m_fMultiplier = 1.0f;
    plUInt32 m_uiCount = 0;
  };

  plHybridArray<MulCount, 8> systemMaxParticles;
  {
    systemMaxParticles.SetCountUninitialized(systems.GetCount());
    for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      plUInt32 uiMaxParticlesAbs = 0, uiMaxParticlesPerSec = 0;
      for (const plParticleEmitterFactory* pEmitter : systems[i]->GetEmitterFactories())
      {
        plUInt32 uiMaxParticlesAbs0 = 0, uiMaxParticlesPerSec0 = 0;
        pEmitter->QueryMaxParticleCount(uiMaxParticlesAbs0, uiMaxParticlesPerSec0);

        uiMaxParticlesAbs += uiMaxParticlesAbs0;
        uiMaxParticlesPerSec += uiMaxParticlesPerSec0;
      }

      const plTime tLifetime = systems[i]->GetAvgLifetime();

      const plUInt32 uiMaxParticles = plMath::Max(32u, plMath::Max(uiMaxParticlesAbs, (plUInt32)(uiMaxParticlesPerSec * tLifetime.GetSeconds())));

      float fMultiplier = 1.0f;

      for (const plParticleInitializerFactory* pInitializer : systems[i]->GetInitializerFactories())
      {
        fMultiplier *= pInitializer->GetSpawnCountMultiplier(this);
      }

      systemMaxParticles[i].m_fMultiplier = plMath::Max(0.0f, fMultiplier);
      systemMaxParticles[i].m_uiCount = (plUInt32)(uiMaxParticles * systemMaxParticles[i].m_fMultiplier);
    }
  }
  // delete all that have important changes
  {
    for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        if (m_ParticleSystems[i]->GetMaxParticles() != systemMaxParticles[i].m_uiCount)
          ClearParticleSystem(i);
      }
    }
  }

  // recreate where necessary
  {
    for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] == nullptr)
      {
        m_ParticleSystems[i] = m_pOwnerModule->CreateSystemInstance(systemMaxParticles[i].m_uiCount, m_pWorld, this, systemMaxParticles[i].m_fMultiplier);
      }
    }
  }

  const plVec3 vStartVelocity = m_vVelocity * m_fApplyInstanceVelocity;

  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    m_ParticleSystems[i]->ConfigureFromTemplate(systems[i]);
    m_ParticleSystems[i]->SetTransform(m_Transform, vStartVelocity);
    m_ParticleSystems[i]->SetEmitterEnabled(m_bEmitterEnabled);
    m_ParticleSystems[i]->Finalize();
  }

  // recreate event reactions
  {
    ClearEventReactions();

    m_EventReactions.SetCount(desc.GetEventReactions().GetCount());

    const auto& er = desc.GetEventReactions();
    for (plUInt32 i = 0; i < er.GetCount(); ++i)
    {
      if (m_EventReactions[i] == nullptr)
      {
        m_EventReactions[i] = er[i]->CreateEventReaction(this);
      }
    }
  }
}

bool plParticleEffectInstance::Update(const plTime& diff)
{
  PL_PROFILE_SCOPE("PFX: Effect Update");

  plTime tMinStep = plTime::MakeFromSeconds(0);

  if (!IsVisible() && m_iMinSimStepsToDo == 0)
  {
    // shared effects always get paused when they are invisible
    if (IsSharedEffect())
      return true;

    switch (m_InvisibleUpdateRate)
    {
      case plEffectInvisibleUpdateRate::FullUpdate:
        tMinStep = plTime::MakeFromSeconds(1.0 / 60.0);
        break;

      case plEffectInvisibleUpdateRate::Max20fps:
        tMinStep = plTime::MakeFromMilliseconds(50);
        break;

      case plEffectInvisibleUpdateRate::Max10fps:
        tMinStep = plTime::MakeFromMilliseconds(100);
        break;

      case plEffectInvisibleUpdateRate::Max5fps:
        tMinStep = plTime::MakeFromMilliseconds(200);
        break;

      case plEffectInvisibleUpdateRate::Pause:
      {
        if (m_bEmitterEnabled)
        {
          // during regular operation, pause
          return m_uiReviveTimeout > 0;
        }

        // otherwise do infrequent updates to shut the effect down
        tMinStep = plTime::MakeFromMilliseconds(200);
        break;
      }

      case plEffectInvisibleUpdateRate::Discard:
        Interrupt();
        return false;
    }
  }

  m_ElapsedTimeSinceUpdate += diff;
  PassTransformToSystems();

  // if the time step is too big, iterate multiple times
  {
    const plTime tMaxTimeStep = plTime::MakeFromMilliseconds(200); // in sync with Max5fps
    while (m_ElapsedTimeSinceUpdate > tMaxTimeStep)
    {
      m_ElapsedTimeSinceUpdate -= tMaxTimeStep;

      if (!StepSimulation(tMaxTimeStep))
        return false;
    }
  }

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return m_uiReviveTimeout > 0;

  // do the remainder
  const plTime tUpdateDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate = plTime::MakeZero();

  return StepSimulation(tUpdateDiff);
}

bool plParticleEffectInstance::StepSimulation(const plTime& tDiff)
{
  m_TotalEffectLifeTime += tDiff;

  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i] != nullptr)
    {
      auto state = m_ParticleSystems[i]->Update(tDiff);

      if (state == plParticleSystemState::Inactive)
      {
        ClearParticleSystem(i);
      }
      else if (state != plParticleSystemState::OnlyReacting)
      {
        // this is used to delay particle effect death by a couple of frames
        // that way, if an event is in the pipeline that might trigger a reacting emitter,
        // or particles are in the spawn queue, but not yet created, we don't kill the effect too early
        m_uiReviveTimeout = 3;
      }
    }
  }

  CombineSystemBoundingVolumes();

  m_iMinSimStepsToDo = plMath::Max<plInt8>(m_iMinSimStepsToDo - 1, 0);

  --m_uiReviveTimeout;
  return m_uiReviveTimeout > 0;
}


void plParticleEffectInstance::AddParticleEvent(const plParticleEvent& pe)
{
  // drop events when the capacity is full
  if (m_EventQueue.GetCount() == m_EventQueue.GetCapacity())
    return;

  m_EventQueue.PushBack(pe);
}

void plParticleEffectInstance::UpdateWindSamples()
{
  const plUInt32 uiDataIdx = plRenderWorld::GetDataIndexForExtraction();

  m_vSampleWindResults[uiDataIdx].Clear();

  if (m_vSampleWindLocations[uiDataIdx].IsEmpty())
    return;

  m_vSampleWindResults[uiDataIdx].SetCount(m_vSampleWindLocations[uiDataIdx].GetCount(), plVec3::MakeZero());

  if (auto pWind = GetWorld()->GetModuleReadOnly<plWindWorldModuleInterface>())
  {
    for (plUInt32 i = 0; i < m_vSampleWindLocations[uiDataIdx].GetCount(); ++i)
    {
      m_vSampleWindResults[uiDataIdx][i] = pWind->GetWindAt(m_vSampleWindLocations[uiDataIdx][i]);
    }
  }

  m_vSampleWindLocations[uiDataIdx].Clear();
}

plUInt64 plParticleEffectInstance::GetNumActiveParticles() const
{
  plUInt64 num = 0;

  for (auto pSystem : m_ParticleSystems)
  {
    if (pSystem)
    {
      num += pSystem->GetNumActiveParticles();
    }
  }

  return num;
}

void plParticleEffectInstance::SetTransform(const plTransform& transform, const plVec3& vParticleStartVelocity)
{
  m_Transform = transform;
  m_TransformForNextFrame = transform;

  m_vVelocity = vParticleStartVelocity;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

void plParticleEffectInstance::SetTransformForNextFrame(const plTransform& transform, const plVec3& vParticleStartVelocity)
{
  m_TransformForNextFrame = transform;
  m_vVelocityForNextFrame = vParticleStartVelocity;
}

plInt32 plParticleEffectInstance::AddWindSampleLocation(const plVec3& vPos)
{
  const plUInt32 uiDataIdx = plRenderWorld::GetDataIndexForRendering();

  if (m_vSampleWindLocations[uiDataIdx].GetCount() < m_vSampleWindLocations[uiDataIdx].GetCapacity())
  {
    m_vSampleWindLocations[uiDataIdx].PushBack(vPos);
    return m_vSampleWindLocations[uiDataIdx].GetCount() - 1;
  }

  return -1;
}

plVec3 plParticleEffectInstance::GetWindSampleResult(plInt32 iIdx) const
{
  const plUInt32 uiDataIdx = plRenderWorld::GetDataIndexForRendering();

  if (iIdx >= 0 && m_vSampleWindResults[uiDataIdx].GetCount() > (plUInt32)iIdx)
  {
    return m_vSampleWindResults[uiDataIdx][iIdx];
  }

  return plVec3::MakeZero();
}

void plParticleEffectInstance::PassTransformToSystems()
{
  if (!m_bSimulateInLocalSpace)
  {
    const plVec3 vStartVel = m_vVelocity * m_fApplyInstanceVelocity;

    for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
    {
      if (m_ParticleSystems[i] != nullptr)
      {
        m_ParticleSystems[i]->SetTransform(m_Transform, vStartVel);
      }
    }
  }
}

void plParticleEffectInstance::AddSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Insert(pSharedInstanceOwner);
}

void plParticleEffectInstance::RemoveSharedInstance(const void* pSharedInstanceOwner)
{
  m_SharedInstances.Remove(pSharedInstanceOwner);
}

bool plParticleEffectInstance::ShouldBeUpdated() const
{
  if (m_hEffectHandle.IsInvalidated())
    return false;

  // do not update shared instances when there is no one watching
  if (m_bIsSharedEffect && m_SharedInstances.GetCount() == 0)
    return false;

  return true;
}

void plParticleEffectInstance::GetBoundingVolume(plBoundingBoxSphere& ref_volume) const
{
  if (!m_BoundingVolume.IsValid())
  {
    ref_volume = plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), 0.25f);
    return;
  }

  ref_volume = m_BoundingVolume;

  if (!m_bSimulateInLocalSpace)
  {
    // transform the bounding volume to local space, unless it was already created there
    const plMat4 invTrans = GetTransform().GetAsMat4().GetInverse();
    ref_volume.Transform(invTrans);
  }
}

void plParticleEffectInstance::CombineSystemBoundingVolumes()
{
  plBoundingBoxSphere effectVolume = plBoundingBoxSphere::MakeInvalid();

  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      const plBoundingBoxSphere& systemVolume = m_ParticleSystems[i]->GetBoundingVolume();
      if (systemVolume.IsValid())
      {
        effectVolume.ExpandToInclude(systemVolume);
      }
    }
  }

  m_BoundingVolume = effectVolume;
}

void plParticleEffectInstance::ProcessEventQueues()
{
  m_Transform = m_TransformForNextFrame;
  m_vVelocity = m_vVelocityForNextFrame;

  if (m_EventQueue.IsEmpty())
    return;

  PL_PROFILE_SCOPE("PFX: Effect Event Queue");
  for (plUInt32 i = 0; i < m_ParticleSystems.GetCount(); ++i)
  {
    if (m_ParticleSystems[i])
    {
      m_ParticleSystems[i]->ProcessEventQueue(m_EventQueue);
    }
  }

  for (const plParticleEvent& e : m_EventQueue)
  {
    plUInt32 rnd = m_Random.UIntInRange(100);

    for (plParticleEventReaction* pReaction : m_EventReactions)
    {
      if (pReaction->m_sEventName != e.m_EventType)
        continue;

      if (pReaction->m_uiProbability > rnd)
      {
        pReaction->ProcessEvent(e);
        break;
      }

      rnd -= pReaction->m_uiProbability;
    }
  }

  m_EventQueue.Clear();
}

plParticleEffectUpdateTask::plParticleEffectUpdateTask(plParticleEffectInstance* pEffect)
{
  m_pEffect = pEffect;
  m_UpdateDiff = plTime::MakeZero();
}

void plParticleEffectUpdateTask::Execute()
{
  if (HasBeenCanceled())
    return;

  if (m_UpdateDiff.GetSeconds() != 0.0)
  {
    m_pEffect->PreSimulate();

    if (!m_pEffect->Update(m_UpdateDiff))
    {
      const plParticleEffectHandle hEffect = m_pEffect->GetHandle();
      PL_ASSERT_DEBUG(!hEffect.IsInvalidated(), "Invalid particle effect handle");

      m_pEffect->GetOwnerWorldModule()->DestroyEffectInstance(hEffect, true, nullptr);
    }
  }
}

void plParticleEffectInstance::SetParameter(const plTempHashedString& sName, float value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (plUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
    {
      m_FloatParameters[i].m_fValue = value;
      return;
    }
  }

  auto& ref = m_FloatParameters.ExpandAndGetRef();
  ref.m_uiNameHash = sName.GetHash();
  ref.m_fValue = value;
}

void plParticleEffectInstance::SetParameter(const plTempHashedString& sName, const plColor& value)
{
  // shared effects do not support parameters
  if (m_bIsSharedEffect)
    return;

  for (plUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
    {
      m_ColorParameters[i].m_Value = value;
      return;
    }
  }

  auto& ref = m_ColorParameters.ExpandAndGetRef();
  ref.m_uiNameHash = sName.GetHash();
  ref.m_Value = value;
}

plInt32 plParticleEffectInstance::FindFloatParameter(const plTempHashedString& sName) const
{
  for (plUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
      return i;
  }

  return -1;
}

float plParticleEffectInstance::GetFloatParameter(const plTempHashedString& sName, float fDefaultValue) const
{
  if (sName.IsEmpty())
    return fDefaultValue;

  for (plUInt32 i = 0; i < m_FloatParameters.GetCount(); ++i)
  {
    if (m_FloatParameters[i].m_uiNameHash == sName.GetHash())
      return m_FloatParameters[i].m_fValue;
  }

  return fDefaultValue;
}

plInt32 plParticleEffectInstance::FindColorParameter(const plTempHashedString& sName) const
{
  for (plUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
      return i;
  }

  return -1;
}

const plColor& plParticleEffectInstance::GetColorParameter(const plTempHashedString& sName, const plColor& defaultValue) const
{
  if (sName.IsEmpty())
    return defaultValue;

  for (plUInt32 i = 0; i < m_ColorParameters.GetCount(); ++i)
  {
    if (m_ColorParameters[i].m_uiNameHash == sName.GetHash())
      return m_ColorParameters[i].m_Value;
  }

  return defaultValue;
}

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Effect_ParticleEffectInstance);
