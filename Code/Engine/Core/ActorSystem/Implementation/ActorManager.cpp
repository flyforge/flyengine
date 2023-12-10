#include <Core/CorePCH.h>

#include <Core/ActorSystem/Actor.h>
#include <Core/ActorSystem/ActorApiService.h>
#include <Core/ActorSystem/ActorManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/UniquePtr.h>

//////////////////////////////////////////////////////////////////////////

static plUniquePtr<plActorManager> s_pActorManager;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Core, plActorManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pActorManager = PLASMA_DEFAULT_NEW(plActorManager);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pActorManager.Clear();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pActorManager)
    {
      s_pActorManager->DestroyAllActors(nullptr, plActorManager::DestructionMode::Immediate);
    }
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

struct plActorManagerImpl
{
  plMutex m_Mutex;
  plHybridArray<plUniquePtr<plActor>, 8> m_AllActors;
  plHybridArray<plUniquePtr<plActorApiService>, 8> m_AllApiServices;
};

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_SINGLETON(plActorManager);

plCopyOnBroadcastEvent<const plActorEvent&> plActorManager::s_ActorEvents;

plActorManager::plActorManager()
  : m_SingletonRegistrar(this)
{
  m_pImpl = PLASMA_DEFAULT_NEW(plActorManagerImpl);
}

plActorManager::~plActorManager()
{
  Shutdown();
}

void plActorManager::Shutdown()
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  DestroyAllActors(nullptr, DestructionMode::Immediate);
  DestroyAllApiServices();

  s_ActorEvents.Clear();
}

void plActorManager::AddActor(plUniquePtr<plActor>&& pActor)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  PLASMA_ASSERT_DEV(pActor != nullptr, "Actor must exist to be added.");
  m_pImpl->m_AllActors.PushBack(std::move(pActor));

  plActorEvent e;
  e.m_Type = plActorEvent::Type::AfterActorCreation;
  e.m_pActor = m_pImpl->m_AllActors.PeekBack().Borrow();
  s_ActorEvents.Broadcast(e);
}

void plActorManager::DestroyActor(plActor* pActor, DestructionMode mode)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  pActor->m_State = plActor::State::QueuedForDestruction;

  if (mode == DestructionMode::Immediate && m_bForceQueueActorDestruction == false)
  {
    for (plUInt32 i = 0; i < m_pImpl->m_AllActors.GetCount(); ++i)
    {
      if (m_pImpl->m_AllActors[i] == pActor)
      {
        plActorEvent e;
        e.m_Type = plActorEvent::Type::BeforeActorDestruction;
        e.m_pActor = pActor;
        s_ActorEvents.Broadcast(e);

        m_pImpl->m_AllActors.RemoveAtAndCopy(i);
        break;
      }
    }
  }
}

void plActorManager::DestroyAllActors(const void* pCreatedBy, DestructionMode mode)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  for (plUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const plUInt32 i = i0 - 1;
    plActor* pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pCreatedBy == nullptr || pActor->GetCreatedBy() == pCreatedBy)
    {
      pActor->m_State = plActor::State::QueuedForDestruction;

      if (mode == DestructionMode::Immediate && m_bForceQueueActorDestruction == false)
      {
        plActorEvent e;
        e.m_Type = plActorEvent::Type::BeforeActorDestruction;
        e.m_pActor = pActor;
        s_ActorEvents.Broadcast(e);

        m_pImpl->m_AllActors.RemoveAtAndCopy(i);
      }
    }
  }
}

void plActorManager::GetAllActors(plHybridArray<plActor*, 8>& out_AllActors)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  out_AllActors.Clear();

  for (auto& pActor : m_pImpl->m_AllActors)
  {
    out_AllActors.PushBack(pActor.Borrow());
  }
}

void plActorManager::AddApiService(plUniquePtr<plActorApiService>&& pApiService)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  PLASMA_ASSERT_DEV(pApiService != nullptr, "Invalid API service");
  PLASMA_ASSERT_DEV(pApiService->m_State == plActorApiService::State::New, "Actor API service already in use");

  for (auto& pExisting : m_pImpl->m_AllApiServices)
  {
    PLASMA_ASSERT_ALWAYS(pApiService->GetDynamicRTTI() != pExisting->GetDynamicRTTI() || pExisting->m_State == plActorApiService::State::QueuedForDestruction, "An actor API service of this type has already been added");
  }

  m_pImpl->m_AllApiServices.PushBack(std::move(pApiService));
}

void plActorManager::DestroyApiService(plActorApiService* pApiService, DestructionMode mode /*= DestructionMode::Immediate*/)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  PLASMA_ASSERT_DEV(pApiService != nullptr, "Invalid API service");

  pApiService->m_State = plActorApiService::State::QueuedForDestruction;

  if (mode == DestructionMode::Immediate)
  {
    for (plUInt32 i = 0; i < m_pImpl->m_AllApiServices.GetCount(); ++i)
    {
      if (m_pImpl->m_AllApiServices[i] == pApiService)
      {
        m_pImpl->m_AllApiServices.RemoveAtAndCopy(i);
        break;
      }
    }
  }
}

void plActorManager::DestroyAllApiServices(DestructionMode mode /*= DestructionMode::Immediate*/)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  for (plUInt32 i0 = m_pImpl->m_AllApiServices.GetCount(); i0 > 0; --i0)
  {
    const plUInt32 i = i0 - 1;
    plActorApiService* pApiService = m_pImpl->m_AllApiServices[i].Borrow();

    pApiService->m_State = plActorApiService::State::QueuedForDestruction;

    if (mode == DestructionMode::Immediate)
    {
      m_pImpl->m_AllApiServices.RemoveAtAndCopy(i);
    }
  }
}

void plActorManager::ActivateQueuedApiServices()
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  for (auto& pManager : m_pImpl->m_AllApiServices)
  {
    if (pManager->m_State == plActorApiService::State::New)
    {
      pManager->Activate();
      pManager->m_State = plActorApiService::State::Active;
    }
  }
}

plActorApiService* plActorManager::GetApiService(const plRTTI* pType)
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  PLASMA_ASSERT_DEV(pType->IsDerivedFrom<plActorApiService>(), "The queried type has to derive from plActorApiService");

  for (auto& pApiService : m_pImpl->m_AllApiServices)
  {
    if (pApiService->GetDynamicRTTI()->IsDerivedFrom(pType) && pApiService->m_State != plActorApiService::State::QueuedForDestruction)
      return pApiService.Borrow();
  }

  return nullptr;
}

void plActorManager::UpdateAllApiServices()
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  for (auto& pApiService : m_pImpl->m_AllApiServices)
  {
    if (pApiService->m_State == plActorApiService::State::Active)
    {
      pApiService->Update();
    }
  }
}

void plActorManager::UpdateAllActors()
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  m_bForceQueueActorDestruction = true;
  PLASMA_SCOPE_EXIT(m_bForceQueueActorDestruction = false);

  for (plUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const plUInt32 i = i0 - 1;
    plActor* pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pActor->m_State == plActor::State::New)
    {
      pActor->m_State = plActor::State::Active;

      pActor->Activate();

      plActorEvent e;
      e.m_Type = plActorEvent::Type::AfterActorActivation;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);
    }

    if (pActor->m_State == plActor::State::Active)
    {
      pActor->Update();
    }
  }
}

void plActorManager::DestroyQueuedActors()
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  PLASMA_ASSERT_DEV(!m_bForceQueueActorDestruction, "Cannot execute this function right now");

  for (plUInt32 i0 = m_pImpl->m_AllActors.GetCount(); i0 > 0; --i0)
  {
    const plUInt32 i = i0 - 1;
    plActor* pActor = m_pImpl->m_AllActors[i].Borrow();

    if (pActor->m_State == plActor::State::QueuedForDestruction)
    {
      plActorEvent e;
      e.m_Type = plActorEvent::Type::BeforeActorDestruction;
      e.m_pActor = pActor;
      s_ActorEvents.Broadcast(e);

      m_pImpl->m_AllActors.RemoveAtAndCopy(i);
    }
  }
}

void plActorManager::DestroyQueuedActorApiServices()
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  for (plUInt32 i0 = m_pImpl->m_AllApiServices.GetCount(); i0 > 0; --i0)
  {
    const plUInt32 i = i0 - 1;
    plActorApiService* pApiService = m_pImpl->m_AllApiServices[i].Borrow();

    if (pApiService->m_State == plActorApiService::State::QueuedForDestruction)
    {
      m_pImpl->m_AllApiServices.RemoveAtAndCopy(i);
    }
  }
}

void plActorManager::Update()
{
  PLASMA_LOCK(m_pImpl->m_Mutex);

  DestroyQueuedActorApiServices();
  DestroyQueuedActors();
  ActivateQueuedApiServices();
  UpdateAllApiServices();
  UpdateAllActors();
}


PLASMA_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorManager);
