#pragma once

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/StateMachine/StateMachine.h>

using plStateMachineResourceHandle = plTypedResourceHandle<class plStateMachineResource>;

class PLASMA_GAMEENGINE_DLL plStateMachineResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plStateMachineResource);

public:
  plStateMachineResource();
  ~plStateMachineResource();

  const plSharedPtr<const plStateMachineDescription>& GetDescription() const { return m_pDescription; }

  plUniquePtr<plStateMachineInstance> CreateInstance(plReflectedClass& owner);

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plSharedPtr<const plStateMachineDescription> m_pDescription;
};
