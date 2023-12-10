#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

struct plActorManagerImpl;
class plActor;
class plActorApiService;

struct plActorEvent
{
  enum class Type
  {
    AfterActorCreation,
    AfterActorActivation, ///< Sent after plActor::Activate() has been called
    BeforeActorDestruction,
  };

  Type m_Type;
  plActor* m_pActor = nullptr;
};

class PLASMA_CORE_DLL plActorManager final
{
  PLASMA_DECLARE_SINGLETON(plActorManager);

public:
  plActorManager();
  ~plActorManager();

  static plCopyOnBroadcastEvent<const plActorEvent&> s_ActorEvents;

  /// \brief Updates all Actors and ActorApiServices, deletes actors that are queued for destruction
  void Update();

  /// \brief Destroys all Actors and ActorApiServices.
  void Shutdown();

  /// \brief Specifies whether something should be destroyed right now or delayed during the next Update()
  enum class DestructionMode
  {
    Immediate, ///< Destruction is executed right now
    Queued     ///< Destruction is queued and done during the next Update()
  };

  /// \brief Gives control over the actor to the plActorManager.
  ///
  /// From now on the actor will be updated every frame and the lifetime will be managed by the plActorManager.
  void AddActor(plUniquePtr<plActor>&& pActor);

  /// \brief Destroys the given actor. If mode is DestructionMode::Queued the destruction will be delayed until the end of the next Update().
  void DestroyActor(plActor* pActor, DestructionMode mode = DestructionMode::Immediate);

  /// \brief Destroys all actors which have been created by the pCreatedBy object.
  ///
  /// If pCreatedBy == nullptr, all actors are destroyed.
  /// If mode is DestructionMode::Queued the destruction will be delayed until the end of the next Update().
  void DestroyAllActors(const void* pCreatedBy, DestructionMode mode = DestructionMode::Immediate);

  /// \brief Returns all actors currently in the system, including ones that are queued for destruction.
  void GetAllActors(plHybridArray<plActor*, 8>& out_AllActors);

  /// \brief Destroys all actors that are queued for destruction.
  /// This is already executed by Update(), calling it directly only makes sense if one needs to clean up actors without also updating the others.
  void DestroyQueuedActors();

  void AddApiService(plUniquePtr<plActorApiService>&& pService);
  void DestroyApiService(plActorApiService* pService, DestructionMode mode = DestructionMode::Immediate);
  void DestroyAllApiServices(DestructionMode mode = DestructionMode::Immediate);
  void DestroyQueuedActorApiServices();

  plActorApiService* GetApiService(const plRTTI* pType);

  template <typename Type>
  Type* GetApiService()
  {
    return static_cast<Type*>(GetApiService(plGetStaticRTTI<Type>()));
  }

private:
  void ActivateQueuedApiServices();
  void UpdateAllApiServices();
  void UpdateAllActors();

  // used during actor updates to force actor destruction to be queued until the actor updating is finished
  bool m_bForceQueueActorDestruction = false;
  plUniquePtr<plActorManagerImpl> m_pImpl;
};
