#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
PL_IMPLEMENT_WORLD_MODULE(plScriptWorldModule);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptWorldModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptWorldModule::plScriptWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
}

plScriptWorldModule::~plScriptWorldModule() = default;

void plScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;

    RegisterUpdateFunction(updateDesc);
  }
}

void plScriptWorldModule::WorldClear()
{
  m_Scheduler.Clear();
}

void plScriptWorldModule::AddUpdateFunctionToSchedule(const plAbstractFunctionProperty* pFunction, void* pInstance, plTime updateInterval, bool bOnlyWhenSimulating)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtrAndFlags(pFunction, bOnlyWhenSimulating ? FunctionContext::Flags::OnlyWhenSimulating : FunctionContext::Flags::None);
  context.m_pInstance = pInstance;

  m_Scheduler.AddOrUpdateWork(context, updateInterval);
}

void plScriptWorldModule::RemoveUpdateFunctionToSchedule(const plAbstractFunctionProperty* pFunction, void* pInstance)
{
  FunctionContext context;
  context.m_pFunctionAndFlags.SetPtr(pFunction);
  context.m_pInstance = pInstance;

  m_Scheduler.RemoveWork(context);
}

plScriptCoroutineHandle plScriptWorldModule::CreateCoroutine(const plRTTI* pCoroutineType, plStringView sName, plScriptInstance& inout_instance, plScriptCoroutineCreationMode::Enum creationMode, plScriptCoroutine*& out_pCoroutine)
{
  if (creationMode != plScriptCoroutineCreationMode::AllowOverlap)
  {
    plScriptCoroutine* pOverlappingCoroutine = nullptr;

    auto& runningCoroutines = m_InstanceToScriptCoroutines[&inout_instance];
    for (auto& hCoroutine : runningCoroutines)
    {
      plUniquePtr<plScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        pOverlappingCoroutine = pCoroutine->Borrow();
        break;
      }
    }

    if (pOverlappingCoroutine != nullptr)
    {
      if (creationMode == plScriptCoroutineCreationMode::StopOther)
      {
        StopAndDeleteCoroutine(pOverlappingCoroutine->GetHandle());
      }
      else if (creationMode == plScriptCoroutineCreationMode::DontCreateNew)
      {
        out_pCoroutine = nullptr;
        return plScriptCoroutineHandle();
      }
      else
      {
        PL_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  auto pCoroutine = pCoroutineType->GetAllocator()->Allocate<plScriptCoroutine>(plScriptAllocator::GetAllocator());

  plScriptCoroutineId id = m_RunningScriptCoroutines.Insert(pCoroutine);
  pCoroutine->Initialize(id, sName, inout_instance, *this);

  m_InstanceToScriptCoroutines[&inout_instance].PushBack(plScriptCoroutineHandle(id));

  out_pCoroutine = pCoroutine;
  return plScriptCoroutineHandle(id);
}

void plScriptWorldModule::StartCoroutine(plScriptCoroutineHandle hCoroutine, plArrayPtr<plVariant> arguments)
{
  plUniquePtr<plScriptCoroutine>* pCoroutine = nullptr;
  if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine))
  {
    (*pCoroutine)->StartWithVarargs(arguments);
    (*pCoroutine)->UpdateAndSchedule();
  }
}

void plScriptWorldModule::StopAndDeleteCoroutine(plScriptCoroutineHandle hCoroutine)
{
  plUniquePtr<plScriptCoroutine> pCoroutine;
  if (m_RunningScriptCoroutines.Remove(hCoroutine.GetInternalID(), &pCoroutine) == false)
    return;

  pCoroutine->Stop();
  pCoroutine->Deinitialize();
  m_DeadScriptCoroutines.PushBack(std::move(pCoroutine));
}

void plScriptWorldModule::StopAndDeleteCoroutine(plStringView sName, plScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (plUInt32 i = 0; i < pCoroutines->GetCount();)
    {
      auto hCoroutine = (*pCoroutines)[i];

      plUniquePtr<plScriptCoroutine>* pCoroutine = nullptr;
      if (m_RunningScriptCoroutines.TryGetValue(hCoroutine.GetInternalID(), pCoroutine) && (*pCoroutine)->GetName() == sName)
      {
        StopAndDeleteCoroutine(hCoroutine);
      }
      else
      {
        ++i;
      }
    }
  }
}

void plScriptWorldModule::StopAndDeleteAllCoroutines(plScriptInstance* pInstance)
{
  if (auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance))
  {
    for (auto hCoroutine : *pCoroutines)
    {
      StopAndDeleteCoroutine(hCoroutine);
    }
  }
}

bool plScriptWorldModule::IsCoroutineFinished(plScriptCoroutineHandle hCoroutine) const
{
  return m_RunningScriptCoroutines.Contains(hCoroutine.GetInternalID()) == false;
}

void plScriptWorldModule::CallUpdateFunctions(const plWorldModule::UpdateContext& context)
{
  plWorld* pWorld = GetWorld();

  plTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = plClock::GetGlobalClock()->GetTimeDiff();
  }

  m_Scheduler.Update(deltaTime,
    [this](const FunctionContext& context, plTime deltaTime) {
      if (GetWorld()->GetWorldSimulationEnabled() || context.m_pFunctionAndFlags.GetFlags() == FunctionContext::Flags::None)
      {
        plVariant args[] = {deltaTime};
        plVariant returnValue;
        context.m_pFunctionAndFlags->Execute(context.m_pInstance, plMakeArrayPtr(args), returnValue);
      }
    });

  // Delete dead coroutines
  for (plUInt32 i = 0; i < m_DeadScriptCoroutines.GetCount(); ++i)
  {
    auto& pCoroutine = m_DeadScriptCoroutines[i];
    plScriptInstance* pInstance = pCoroutine->GetScriptInstance();
    auto pCoroutines = m_InstanceToScriptCoroutines.GetValue(pInstance);
    PL_ASSERT_DEV(pCoroutines != nullptr, "Implementation error");

    pCoroutines->RemoveAndSwap(pCoroutine->GetHandle());
    if (pCoroutines->IsEmpty())
    {
      m_InstanceToScriptCoroutines.Remove(pInstance);
    }

    pCoroutine = nullptr;
  }
  m_DeadScriptCoroutines.Clear();
}


PL_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptWorldModule);

