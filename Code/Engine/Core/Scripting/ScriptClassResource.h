#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptRTTI.h>

class plWorld;
using plScriptClassResourceHandle = plTypedResourceHandle<class plScriptClassResource>;

class PLASMA_CORE_DLL plScriptClassResource : public plResource
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plScriptClassResource, plResource);
  PLASMA_RESOURCE_DECLARE_COMMON_CODE(plScriptClassResource);

public:
  plScriptClassResource();
  ~plScriptClassResource();

  const plSharedPtr<plScriptRTTI>& GetType() const { return m_pType; }

  virtual plUniquePtr<plScriptInstance> Instantiate(plReflectedClass& inout_owner, plWorld* pWorld) const = 0;

protected:
  plSharedPtr<plScriptRTTI> CreateScriptType(plStringView sName, const plRTTI* pBaseType, plScriptRTTI::FunctionList&& functions, plScriptRTTI::MessageHandlerList&& messageHandlers);
  void DeleteScriptType();

  plSharedPtr<plScriptCoroutineRTTI> CreateScriptCoroutineType(plStringView sScriptClassName, plStringView sFunctionName, plUniquePtr<plRTTIAllocator>&& pAllocator);
  void DeleteAllScriptCoroutineTypes();

  plSharedPtr<plScriptRTTI> m_pType;
  plDynamicArray<plSharedPtr<plScriptCoroutineRTTI>> m_CoroutineTypes;
};
