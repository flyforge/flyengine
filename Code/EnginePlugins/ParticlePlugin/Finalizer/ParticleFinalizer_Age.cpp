#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Profiling/Profiling.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizerFactory_Age, 1, plRTTIDefaultAllocator<plParticleFinalizerFactory_Age>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleFinalizer_Age, 1, plRTTIDefaultAllocator<plParticleFinalizer_Age>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleFinalizerFactory_Age::plParticleFinalizerFactory_Age() {}

const plRTTI* plParticleFinalizerFactory_Age::GetFinalizerType() const
{
  return plGetStaticRTTI<plParticleFinalizer_Age>();
}

void plParticleFinalizerFactory_Age::CopyFinalizerProperties(plParticleFinalizer* pObject, bool bFirstTime) const
{
  plParticleFinalizer_Age* pFinalizer = static_cast<plParticleFinalizer_Age*>(pObject);

  pFinalizer->m_LifeTime = m_LifeTime;
  pFinalizer->m_sOnDeathEvent = plTempHashedString(m_sOnDeathEvent.GetData());
  pFinalizer->m_sLifeScaleParameter = plTempHashedString(m_sLifeScaleParameter.GetData());

  if (pFinalizer->m_bHasOnDeathEventHandler)
  {
    pFinalizer->m_bHasOnDeathEventHandler = false;
    pFinalizer->GetOwnerSystem()->RemoveParticleDeathEventHandler(plMakeDelegate(&plParticleFinalizer_Age::OnParticleDeath, pFinalizer));
  }

  if (!pFinalizer->m_sOnDeathEvent.IsEmpty())
  {
    pFinalizer->m_bHasOnDeathEventHandler = true;
    pFinalizer->GetOwnerSystem()->AddParticleDeathEventHandler(plMakeDelegate(&plParticleFinalizer_Age::OnParticleDeath, pFinalizer));
  }
}

plParticleFinalizer_Age::plParticleFinalizer_Age() = default;

plParticleFinalizer_Age::~plParticleFinalizer_Age()
{
  if (m_bHasOnDeathEventHandler)
  {
    GetOwnerSystem()->RemoveParticleDeathEventHandler(plMakeDelegate(&plParticleFinalizer_Age::OnParticleDeath, this));
  }
}

void plParticleFinalizer_Age::CreateRequiredStreams()
{
  CreateStream("LifeTime", plProcessingStream::DataType::Half2, &m_pStreamLifeTime, true);

  m_pStreamPosition = nullptr;
  m_pStreamVelocity = nullptr;

  if (!m_sOnDeathEvent.IsEmpty())
  {
    CreateStream("Position", plProcessingStream::DataType::Float4, &m_pStreamPosition, false);
    CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, false);
  }
}

void plParticleFinalizer_Age::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Age Init");

  plFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<plFloat16Vec2>();
  const float fLifeScale = plMath::Clamp(GetOwnerEffect()->GetFloatParameter(m_sLifeScaleParameter, 1.0f), 0.0f, 2.0f);

  if (m_LifeTime.m_fVariance == 0)
  {
    const float tLifeTime = (fLifeScale * (float)m_LifeTime.m_Value.GetSeconds()) + 0.01f; // make sure it's not zero
    const float tInvLifeTime = 1.0f / tLifeTime;

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
  else // random range
  {
    plRandom& rng = GetRNG();

    for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
    {
      const float tLifeTime =
        (fLifeScale * (float)rng.DoubleVariance(m_LifeTime.m_Value.GetSeconds(), m_LifeTime.m_fVariance)) + 0.01f; // make sure it's not zero
      const float tInvLifeTime = 1.0f / tLifeTime;

      pLifeTime[i].x = tLifeTime;
      pLifeTime[i].y = tInvLifeTime;
    }
  }
}

void plParticleFinalizer_Age::Process(plUInt64 uiNumElements)
{
  PLASMA_PROFILE_SCOPE("PFX: Age");

  plFloat16Vec2* pLifeTime = m_pStreamLifeTime->GetWritableData<plFloat16Vec2>();

  const float tDiff = (float)m_TimeDiff.GetSeconds();

  for (plUInt32 i = 0; i < uiNumElements; ++i)
  {
    pLifeTime[i].x = pLifeTime[i].x - tDiff;

    if (pLifeTime[i].x <= 0)
    {
      pLifeTime[i].x = 0;

      /// \todo Get current element index from iterator ?
      m_pStreamGroup->RemoveElement(i);
    }
  }
}

void plParticleFinalizer_Age::OnParticleDeath(const plStreamGroupElementRemovedEvent& e)
{
  const plVec4* pPosition = m_pStreamPosition->GetData<plVec4>();
  const plVec3* pVelocity = m_pStreamVelocity->GetData<plVec3>();

  plParticleEvent pe;
  pe.m_EventType = m_sOnDeathEvent;
  pe.m_vPosition = pPosition[e.m_uiElementIndex].GetAsVec3();
  pe.m_vDirection = pVelocity[e.m_uiElementIndex];
  pe.m_vNormal.SetZero();

  GetOwnerEffect()->AddParticleEvent(pe);
}



PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Finalizer_ParticleFinalizer_Age);
