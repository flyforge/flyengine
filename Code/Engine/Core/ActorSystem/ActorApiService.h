#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Reflection/Reflection.h>

class PL_CORE_DLL plActorApiService : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plActorApiService, plReflectedClass);
  PL_DISALLOW_COPY_AND_ASSIGN(plActorApiService);

public:
  plActorApiService();
  ~plActorApiService();

protected:
  virtual void Activate() = 0;
  virtual void Update() = 0;

private: // directly accessed by plActorManager
  friend class plActorManager;

  enum class State
  {
    New,
    Active,
    QueuedForDestruction
  };

  State m_State = State::New;
};
