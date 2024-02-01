#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class plActor;

class PL_CORE_DLL plActorPlugin : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plActorPlugin, plReflectedClass);

public:
  plActorPlugin();
  ~plActorPlugin();

  plActor* GetActor() const;

protected:
  friend class plActor;
  virtual void Update() {}

private:
  plActor* m_pOwningActor = nullptr;
};
