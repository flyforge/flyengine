#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

class PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptClassResource : public plScriptClassResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisualScriptClassResource, plScriptClassResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plVisualScriptClassResource);

public:
  plVisualScriptClassResource();
  ~plVisualScriptClassResource();

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual plUniquePtr<plScriptInstance> Instantiate(plReflectedClass& inout_owner, plWorld* pWorld) const override;

  plSharedPtr<plVisualScriptDataStorage> m_pConstantDataStorage;
  plSharedPtr<const plVisualScriptDataDescription> m_pInstanceDataDesc;
  plSharedPtr<plVisualScriptInstanceDataMapping> m_pInstanceDataMapping;
};
