#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

class plComponent;
class plTypeScriptBinding;

class PL_TYPESCRIPTPLUGIN_DLL plTypeScriptInstance : public plScriptInstance
{
public:
  plTypeScriptInstance(plComponent& inout_owner, plWorld* pWorld, plTypeScriptBinding& inout_binding);

  virtual void SetInstanceVariables(const plArrayMap<plHashedString, plVariant>& parameters) override;
  virtual void SetInstanceVariable(const plHashedString& sName, const plVariant& value) override;
  virtual plVariant GetInstanceVariable(const plHashedString& sName) override;

  plTypeScriptBinding& GetBinding() { return m_Binding; }

  plComponent& GetComponent() { return static_cast<plComponent&>(GetOwner()); }

private:
  plTypeScriptBinding& m_Binding;
};

class PL_TYPESCRIPTPLUGIN_DLL plTypeScriptClassResource : public plScriptClassResource
{
  PL_ADD_DYNAMIC_REFLECTION(plTypeScriptClassResource, plScriptClassResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plTypeScriptClassResource);

public:
  plTypeScriptClassResource();
  ~plTypeScriptClassResource();

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* pStream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  virtual plUniquePtr<plScriptInstance> Instantiate(plReflectedClass& inout_owner, plWorld* pWorld) const override;

private:
  plUuid m_Guid;
};
