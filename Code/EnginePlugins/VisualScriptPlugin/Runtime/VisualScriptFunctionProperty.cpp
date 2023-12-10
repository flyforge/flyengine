#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <VisualScriptPlugin/Runtime/VisualScriptFunctionProperty.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>

plVisualScriptFunctionProperty::plVisualScriptFunctionProperty(plStringView sName, const plSharedPtr<const plVisualScriptGraphDescription>& pDesc)
  : plScriptFunctionProperty(sName)
  , m_pDesc(pDesc)
  , m_LocalDataStorage(pDesc->GetLocalDataDesc())
{
  PLASMA_ASSERT_DEBUG(m_pDesc->IsCoroutine() == false, "Must not be a coroutine");

  m_LocalDataStorage.AllocateStorage();
}

plVisualScriptFunctionProperty::~plVisualScriptFunctionProperty() = default;

void plVisualScriptFunctionProperty::Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const
{
  PLASMA_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pVisualScriptInstance = static_cast<plVisualScriptInstance*>(pInstance);

  plVisualScriptExecutionContext context(m_pDesc);
  context.Initialize(*pVisualScriptInstance, m_LocalDataStorage, arguments);

  auto result = context.Execute(plTime::MakeZero());
  PLASMA_ASSERT_DEBUG(result.m_NextExecAndState != plVisualScriptExecutionContext::ExecResult::State::ContinueLater, "A non-coroutine function must not return 'ContinueLater'");

  // TODO: return value
}

//////////////////////////////////////////////////////////////////////////

plVisualScriptMessageHandler::plVisualScriptMessageHandler(const plScriptMessageDesc& desc, const plSharedPtr<const plVisualScriptGraphDescription>& pDesc)
  : plScriptMessageHandler(desc)
  , m_pDesc(pDesc)
  , m_LocalDataStorage(pDesc->GetLocalDataDesc())
{
  PLASMA_ASSERT_DEBUG(m_pDesc->IsCoroutine() == false, "Must not be a coroutine");

  m_DispatchFunc = &Dispatch;
  m_LocalDataStorage.AllocateStorage();
}

plVisualScriptMessageHandler::~plVisualScriptMessageHandler() = default;

// static
void plVisualScriptMessageHandler::Dispatch(plAbstractMessageHandler* pSelf, void* pInstance, plMessage& ref_msg)
{
  auto pHandler = static_cast<plVisualScriptMessageHandler*>(pSelf);
  auto pComponent = static_cast<plScriptComponent*>(pInstance);
  auto pVisualScriptInstance = static_cast<plVisualScriptInstance*>(pComponent->GetScriptInstance());

  plHybridArray<plVariant, 8> arguments;
  pHandler->FillMessagePropertyValues(ref_msg, arguments);

  plVisualScriptExecutionContext context(pHandler->m_pDesc);
  context.Initialize(*pVisualScriptInstance, pHandler->m_LocalDataStorage, arguments);

  auto result = context.Execute(plTime::MakeZero());
  PLASMA_ASSERT_DEBUG(result.m_NextExecAndState != plVisualScriptExecutionContext::ExecResult::State::ContinueLater, "A non-coroutine function must not return 'ContinueLater'");
}
