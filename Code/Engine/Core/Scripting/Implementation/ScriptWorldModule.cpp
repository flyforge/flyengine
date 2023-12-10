#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClassResource.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
PLASMA_IMPLEMENT_WORLD_MODULE(plScriptWorldModule);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plScriptWorldModule, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plScriptWorldModule::plScriptWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plScriptWorldModule::ResourceEventHandler, this));
}

plScriptWorldModule::~plScriptWorldModule()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plScriptWorldModule::ResourceEventHandler, this));
}

void plScriptWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plScriptWorldModule::CallUpdateFunctions, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plScriptWorldModule::ReloadScripts, this);
    updateDesc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    updateDesc.m_fPriority = 10000.0f;

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
        PLASMA_ASSERT_NOT_IMPLEMENTED;
      }
    }
  }

  auto pCoroutine = pCoroutineType->GetAllocator()->Allocate<plScriptCoroutine>(plFoundation::GetDefaultAllocator());

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
    (*pCoroutine)->Start(arguments);
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

void plScriptWorldModule::AddScriptReloadFunction(plScriptClassResourceHandle hScript, ReloadFunction function)
{
  if (hScript.IsValid() == false)
    return;

  PLASMA_ASSERT_DEV(function.IsComparable(), "Function must be comparable otherwise it can't be removed");
  m_ReloadFunctions[hScript].PushBack(function);
}

void plScriptWorldModule::RemoveScriptReloadFunction(plScriptClassResourceHandle hScript, ReloadFunction function)
{
  PLASMA_ASSERT_DEV(function.IsComparable(), "Function must be comparable otherwise it can't be removed");

  ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_ReloadFunctions.TryGetValue(hScript, pReloadFunctions))
  {
    for (plUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      if ((*pReloadFunctions)[i].IsEqualIfComparable(function))
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

void plScriptWorldModule::CallUpdateFunctions(const plWorldModule::UpdateContext& context)
{
  plWorld* pWorld = GetWorld();

  plTime deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = GetWorld()->GetClock().GetTimeDiff();
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
    PLASMA_ASSERT_DEV(pCoroutines != nullptr, "Implementation error");

    pCoroutines->RemoveAndSwap(pCoroutine->GetHandle());
    if (pCoroutines->IsEmpty())
    {
      m_InstanceToScriptCoroutines.Remove(pInstance);
    }

    pCoroutine = nullptr;
  }
  m_DeadScriptCoroutines.Clear();
}

void plScriptWorldModule::ReloadScripts(const plWorldModule::UpdateContext& context)
{
  for (const auto hScript : m_NeedReload)
  {
    if (m_ReloadFunctions.TryGetValue(hScript, m_TempReloadFunctions))
    {
      for (auto& reloadFunction : m_TempReloadFunctions)
      {
        reloadFunction();
      }
    }
  }

  m_NeedReload.Clear();
}

void plScriptWorldModule::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type != plResourceEvent::Type::ResourceContentUnloading)
    return;

  if (auto pResource = plDynamicCast<const plScriptClassResource*>(e.m_pResource))
  {
    if (pResource->GetReferenceCount() > 0)
    {
      plScriptClassResourceHandle hScript = pResource->GetResourceHandle();
      m_NeedReload.Insert(hScript);
    }
  }
}
