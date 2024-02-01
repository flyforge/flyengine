#pragma once

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/StateMachine/StateMachine.h>

using plStateMachineResourceHandle = plTypedResourceHandle<class plStateMachineResource>;

class PL_GAMEENGINE_DLL plStateMachineResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plStateMachineResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plStateMachineResource);

public:
  plStateMachineResource();
  ~plStateMachineResource();

  const plSharedPtr<const plStateMachineDescription>& GetDescription() const { return m_pDescription; }

  plUniquePtr<plStateMachineInstance> CreateInstance(plReflectedClass& ref_owner);

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  plSharedPtr<const plStateMachineDescription> m_pDescription;
};
