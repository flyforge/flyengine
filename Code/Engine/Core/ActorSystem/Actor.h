#pragma once

#include <Core/ActorSystem/ActorPlugin.h>
#include <Foundation/Types/UniquePtr.h>

struct plActorImpl;

class PLASMA_CORE_DLL plActor : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActor, plReflectedClass);

  PLASMA_DISALLOW_COPY_AND_ASSIGN(plActor);

public:
  plActor(plStringView sActorName, const void* pCreatedBy);
  ~plActor();

  /// \brief Returns the name of this actor
  plStringView GetName() const;

  /// \brief Returns the 'created by' pointer of the actor
  const void* GetCreatedBy() const;

  /// \brief Transfers ownership of the plActorPlugin to the plActor
  void AddPlugin(plUniquePtr<plActorPlugin>&& pPlugin);

  /// \brief Queries the plActor for an plActorPlugin of the given type. Returns null if no such plugin was added to the actor.
  plActorPlugin* GetPlugin(const plRTTI* pType) const;

  /// \brief Templated overload of GetPlugin() that automatically casts to the desired class type.
  template <typename Type>
  Type* GetPlugin() const
  {
    return static_cast<Type*>(GetPlugin(plGetStaticRTTI<Type>()));
  }

  /// \brief Deletes the given plugin from the actor
  void DestroyPlugin(plActorPlugin* pPlugin);

  /// \brief Fills the list with all plugins that have been added to the actor.
  void GetAllPlugins(plHybridArray<plActorPlugin*, 8>& out_AllPlugins);

  /// \brief Checks whether the actor is queued for destruction at the end of the frame
  bool IsActorQueuedForDestruction() const
  {
    return m_State == State::QueuedForDestruction;
  }

protected:
  void UpdateAllPlugins();


protected: // directly touched by plActorManager
  friend class plActorManager;

  /// \brief Called shortly before the first call to Update()
  virtual void Activate();

  /// \brief Called once per frame to update the actor state.
  ///
  /// By default this calls UpdateAllPlugins() internally.
  virtual void Update();

private: // directly touched by plActorManager
  enum class State
  {
    New,
    Active,
    QueuedForDestruction
  };

  State m_State = State::New;

private:
  plUniquePtr<plActorImpl> m_pImpl;
};
