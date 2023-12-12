#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/Streams/ParticleStream.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PLASMA_IMPLEMENT_WORLD_MODULE(plParticleWorldModule);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleWorldModule, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleWorldModule::plParticleWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
}

plParticleWorldModule::~plParticleWorldModule()
{
  ClearParticleStreamFactories();
}

void plParticleWorldModule::Initialize()
{
  ConfigureParticleStreamFactories();

  {
    auto updateDesc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plParticleWorldModule::UpdateEffects, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;
    updateDesc.m_fPriority = 1000.0f; // kick off particle tasks as early as possible

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto finishDesc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plParticleWorldModule::EnsureUpdatesFinished, this);
    finishDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    finishDesc.m_bOnlyUpdateWhenSimulating = true;
    finishDesc.m_fPriority = -1000.0f; // sync with particle tasks as late as possible

    RegisterUpdateFunction(finishDesc);
  }

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plParticleWorldModule::ResourceEventHandler, this));

  plRTTI::ForEachDerivedType<plParticleModule>(
    [this](const plRTTI* pRtti) {
      plUniquePtr<plParticleModule> pModule = pRtti->GetAllocator()->Allocate<plParticleModule>();
      pModule->RequestRequiredWorldModulesForCache(this);
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);
}


void plParticleWorldModule::Deinitialize()
{
  PLASMA_LOCK(m_Mutex);

  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plParticleWorldModule::ResourceEventHandler, this));

  WorldClear();
}

void plParticleWorldModule::EnsureUpdatesFinished(const plWorldModule::UpdateContext& context)
{
  // do NOT lock here, otherwise tasks cannot enter the lock
  plTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  {
    PLASMA_LOCK(m_Mutex);

    // The simulation tasks are done and the game objects have their global transform updated at this point, so we can push the transform
    // to the particle effects for the next simulation step and also ensure that the bounding volumes are correct for culling and rendering.
    if (plParticleComponentManager* pManager = GetWorld()->GetComponentManager<plParticleComponentManager>())
    {
      pManager->UpdatePfxTransformsAndBounds();
    }

    if (plParticleFinisherComponentManager* pManager = GetWorld()->GetComponentManager<plParticleFinisherComponentManager>())
    {
      pManager->UpdateBounds();
    }

    for (plUInt32 i = 0; i < m_NeedFinisherComponent.GetCount(); ++i)
    {
      CreateFinisherComponent(m_NeedFinisherComponent[i]);
    }

    m_NeedFinisherComponent.Clear();
  }
}

void plParticleWorldModule::ExtractEffectRenderData(const plParticleEffectInstance* pEffect, plMsgExtractRenderData& msg, const plTransform& systemTransform) const
{
  PLASMA_ASSERT_DEBUG(plTaskSystem::IsTaskGroupFinished(m_EffectUpdateTaskGroup), "Particle Effect Update Task is not finished!");

  PLASMA_LOCK(m_Mutex);

  for (plUInt32 i = 0; i < pEffect->GetParticleSystems().GetCount(); ++i)
  {
    const plParticleSystemInstance* pSystem = pEffect->GetParticleSystems()[i];

    if (pSystem == nullptr)
      continue;

    if (!pSystem->HasActiveParticles() || !pSystem->IsVisible())
      continue;

    pSystem->ExtractSystemRenderData(msg, systemTransform);
  }
}

void plParticleWorldModule::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plParticleEffectResource>())
  {
    PLASMA_LOCK(m_Mutex);

    plParticleEffectResourceHandle hResource((plParticleEffectResource*)(e.m_pResource));

    const plUInt32 numEffects = m_ParticleEffects.GetCount();
    for (plUInt32 i = 0; i < numEffects; ++i)
    {
      if (m_ParticleEffects[i].GetResource() == hResource)
      {
        m_EffectsToReconfigure.PushBack(&m_ParticleEffects[i]);
      }
    }
  }
}

void plParticleWorldModule::ConfigureParticleStreamFactories()
{
  ClearParticleStreamFactories();

  plStringBuilder fullName;

  plRTTI::ForEachDerivedType<plParticleStreamFactory>(
    [&](const plRTTI* pRtti) {
      plParticleStreamFactory* pFactory = pRtti->GetAllocator()->Allocate<plParticleStreamFactory>();

      plParticleStreamFactory::GetFullStreamName(pFactory->GetStreamName(), pFactory->GetStreamDataType(), fullName);

      m_StreamFactories[fullName] = pFactory;
    },
    plRTTI::ForEachOptions::ExcludeNonAllocatable);
}

void plParticleWorldModule::ClearParticleStreamFactories()
{
  for (auto it : m_StreamFactories)
  {
    it.Value()->GetDynamicRTTI()->GetAllocator()->Deallocate(it.Value());
  }

  m_StreamFactories.Clear();
}

plParticleStream* plParticleWorldModule::CreateStreamDefaultInitializer(plParticleSystemInstance* pOwner, const char* szFullStreamName) const
{
  auto it = m_StreamFactories.Find(szFullStreamName);
  if (!it.IsValid())
    return nullptr;

  return it.Value()->CreateParticleStream(pOwner);
}

plWorldModule* plParticleWorldModule::GetCachedWorldModule(const plRTTI* pRtti) const
{
  plWorldModule* pModule = nullptr;
  m_WorldModuleCache.TryGetValue(pRtti, pModule);
  return pModule;
}

void plParticleWorldModule::CacheWorldModule(const plRTTI* pRtti)
{
  m_WorldModuleCache[pRtti] = GetWorld()->GetOrCreateModule(pRtti);
}

void plParticleWorldModule::CreateFinisherComponent(plParticleEffectInstance* pEffect)
{
  if (pEffect && !pEffect->IsSharedEffect())
  {
    pEffect->SetVisibleIf(nullptr);

    plWorld* pWorld = GetWorld();

    const plTransform transform = pEffect->GetTransform();

    plGameObjectDesc go;
    go.m_LocalPosition = transform.m_vPosition;
    go.m_LocalRotation = transform.m_qRotation;
    go.m_LocalScaling = transform.m_vScale;
    // go.m_Tags = GetOwner()->GetTags(); // TODO: pass along tags -> needed for rendering filters

    plGameObject* pFinisher;
    pWorld->CreateObject(go, pFinisher);

    plParticleFinisherComponent* pFinisherComp;
    plParticleFinisherComponent::CreateComponent(pFinisher, pFinisherComp);

    pFinisherComp->m_EffectController = plParticleEffectController(this, pEffect->GetHandle());
    pFinisherComp->m_EffectController.SetTransform(transform, plVec3::ZeroVector()); // clear the velocity
  }
}

void plParticleWorldModule::WorldClear()
{
  // make sure no particle update task is still in the pipeline
  plTaskSystem::WaitForGroup(m_EffectUpdateTaskGroup);

  PLASMA_LOCK(m_Mutex);

  m_FinishingEffects.Clear();
  m_NeedFinisherComponent.Clear();

  m_ActiveEffects.Clear();
  m_ParticleEffects.Clear();
  m_ParticleSystems.Clear();
  m_ParticleEffectsFreeList.Clear();
  m_ParticleSystemFreeList.Clear();
}

PLASMA_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleWorldModule);
