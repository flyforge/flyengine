#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScript.h>

class PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptCoroutine : public plScriptCoroutine
{
public:
  plVisualScriptCoroutine(const plSharedPtr<const plVisualScriptGraphDescription>& pDesc);
  ~plVisualScriptCoroutine();

  virtual void Start(plArrayPtr<plVariant> arguments) override;
  virtual void Stop() override;
  virtual Result Update(plTime deltaTimeSinceLastUpdate) override;

private:
  plVisualScriptDataStorage m_LocalDataStorage;
  plVisualScriptExecutionContext m_Context;
};

class PLASMA_VISUALSCRIPTPLUGIN_DLL plVisualScriptCoroutineAllocator : public plRTTIAllocator
{
public:
  plVisualScriptCoroutineAllocator(const plSharedPtr<const plVisualScriptGraphDescription>& pDesc);

  void Deallocate(void* pObject, plAllocatorBase* pAllocator = nullptr) override;
  plInternal::NewInstance<void> AllocateInternal(plAllocatorBase* pAllocator) override;

private:
  plSharedPtr<const plVisualScriptGraphDescription> m_pDesc;
};
