#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <VisualScriptPlugin/Runtime/VisualScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

plVisualScriptCoroutine::plVisualScriptCoroutine(const plSharedPtr<const plVisualScriptGraphDescription>& pDesc)
  : m_LocalDataStorage(pDesc->GetLocalDataDesc())
  , m_Context(pDesc)
{
}

plVisualScriptCoroutine::~plVisualScriptCoroutine() = default;

void plVisualScriptCoroutine::StartWithVarargs(plArrayPtr<plVariant> arguments)
{
  m_LocalDataStorage.AllocateStorage();

  auto pVisualScriptInstance = static_cast<plVisualScriptInstance*>(GetScriptInstance());
  m_Context.Initialize(*pVisualScriptInstance, m_LocalDataStorage, arguments);
}

void plVisualScriptCoroutine::Stop()
{
  m_Context.Deinitialize();
}

plScriptCoroutine::Result plVisualScriptCoroutine::Update(plTime deltaTimeSinceLastUpdate)
{
  auto result = m_Context.Execute(deltaTimeSinceLastUpdate);
  if (result.m_NextExecAndState == plVisualScriptExecutionContext::ExecResult::State::ContinueLater)
  {
    return Result::Running(result.m_MaxDelay);
  }

  return Result::Completed();
}

//////////////////////////////////////////////////////////////////////////

plVisualScriptCoroutineAllocator::plVisualScriptCoroutineAllocator(const plSharedPtr<const plVisualScriptGraphDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

void plVisualScriptCoroutineAllocator::Deallocate(void* pObject, plAllocator* pAllocator /*= nullptr*/)
{
  PL_REPORT_FAILURE("Deallocate is not supported");
}

plInternal::NewInstance<void> plVisualScriptCoroutineAllocator::AllocateInternal(plAllocator* pAllocator)
{
  return PL_SCRIPT_NEW(plVisualScriptCoroutine, m_pDesc);
}
