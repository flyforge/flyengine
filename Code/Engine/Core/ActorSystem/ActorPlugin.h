#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class plActor;

class PLASMA_CORE_DLL plActorPlugin : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plActorPlugin, plReflectedClass);

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
