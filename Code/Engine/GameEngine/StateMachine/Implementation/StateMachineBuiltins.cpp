#include <GameEngine/GameEnginePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachineBuiltins.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineState_NestedStateMachine, 1, plRTTIDefaultAllocator<plStateMachineState_NestedStateMachine>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_StateMachine", plDependencyFlags::Package)),
    PL_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    PL_MEMBER_PROPERTY("KeepCurrentStateOnExit", m_bKeepCurrentStateOnExit),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineState_NestedStateMachine::plStateMachineState_NestedStateMachine(plStringView sName)
  : plStateMachineState(sName)
{
}

plStateMachineState_NestedStateMachine::~plStateMachineState_NestedStateMachine() = default;

void plStateMachineState_NestedStateMachine::OnEnter(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pFromState) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;

  if (pStateMachineInstance == nullptr)
  {
    if (m_hResource.IsValid() == false)
      return;

    plResourceLock<plStateMachineResource> pStateMachineResource(m_hResource, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pStateMachineResource.GetAcquireResult() != plResourceAcquireResult::Final)
    {
      plLog::Error("Failed to load state machine '{}'", GetResourceFile());
      return;
    }

    pStateMachineInstance = pStateMachineResource->CreateInstance(ref_instance.GetOwner());
    pStateMachineInstance->SetBlackboard(ref_instance.GetBlackboard());
  }

  if (pStateMachineInstance->GetCurrentState() == nullptr)
  {
    pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
  }
}

void plStateMachineState_NestedStateMachine::OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const
{
  if (m_bKeepCurrentStateOnExit == false)
  {
    auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
    if (pStateMachineInstance != nullptr)
    {
      pStateMachineInstance->SetState(nullptr).IgnoreResult();
    }
  }
}

void plStateMachineState_NestedStateMachine::Update(plStateMachineInstance& ref_instance, void* pInstanceData, plTime deltaTime) const
{
  auto& pStateMachineInstance = static_cast<InstanceData*>(pInstanceData)->m_pStateMachineInstance;
  if (pStateMachineInstance != nullptr)
  {
    pStateMachineInstance->Update(deltaTime);
  }
}

plResult plStateMachineState_NestedStateMachine::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_hResource;
  inout_stream << m_sInitialState;
  inout_stream << m_bKeepCurrentStateOnExit;
  return PL_SUCCESS;
}

plResult plStateMachineState_NestedStateMachine::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_hResource;
  inout_stream >> m_sInitialState;
  inout_stream >> m_bKeepCurrentStateOnExit;
  return PL_SUCCESS;
}

bool plStateMachineState_NestedStateMachine::GetInstanceDataDesc(plInstanceDataDesc& out_desc)
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

void plStateMachineState_NestedStateMachine::SetResource(const plStateMachineResourceHandle& hResource)
{
  m_hResource = hResource;
}

void plStateMachineState_NestedStateMachine::SetResourceFile(const char* szFile)
{
  plStateMachineResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plStateMachineResource>(szFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* plStateMachineState_NestedStateMachine::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void plStateMachineState_NestedStateMachine::SetInitialState(const char* szName)
{
  m_sInitialState.Assign(szName);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineState_Compound, 1, plRTTIDefaultAllocator<plStateMachineState_Compound>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("SubStates", m_SubStates)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineState_Compound::plStateMachineState_Compound(plStringView sName)
  : plStateMachineState(sName)
{
}

plStateMachineState_Compound::~plStateMachineState_Compound()
{
  for (auto pSubState : m_SubStates)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

void plStateMachineState_Compound::OnEnter(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pFromState) const
{
  auto pData = static_cast<plStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  for (plUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnEnter(ref_instance, pSubInstanceData, pFromState);
  }
}

void plStateMachineState_Compound::OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const
{
  auto pData = static_cast<plStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (plUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->OnExit(ref_instance, pSubInstanceData, pToState);
  }
}

void plStateMachineState_Compound::Update(plStateMachineInstance& ref_instance, void* pInstanceData, plTime deltaTime) const
{
  auto pData = static_cast<plStateMachineInternal::Compound::InstanceData*>(pInstanceData);

  for (plUInt32 i = 0; i < m_SubStates.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    m_SubStates[i]->Update(ref_instance, pSubInstanceData, deltaTime);
  }
}

plResult plStateMachineState_Compound::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  const plUInt32 uiNumSubStates = m_SubStates.GetCount();
  inout_stream << uiNumSubStates;

  for (auto pSubState : m_SubStates)
  {
    auto pStateType = pSubState->GetDynamicRTTI();
    plTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    PL_SUCCEED_OR_RETURN(pSubState->Serialize(inout_stream));
  }

  return PL_SUCCESS;
}

plResult plStateMachineState_Compound::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);

  plUInt32 uiNumSubStates = 0;
  inout_stream >> uiNumSubStates;
  m_SubStates.Reserve(uiNumSubStates);

  plStringBuilder sTypeName;
  for (plUInt32 i = 0; i < uiNumSubStates; ++i)
  {
    inout_stream >> sTypeName;
    if (const plRTTI* pType = plRTTI::FindTypeByName(sTypeName))
    {
      plUniquePtr<plStateMachineState> pSubState = pType->GetAllocator()->Allocate<plStateMachineState>();
      PL_SUCCEED_OR_RETURN(pSubState->Deserialize(inout_stream));

      m_SubStates.PushBack(pSubState.Release());
    }
    else
    {
      plLog::Error("Unknown state machine state type '{}'", sTypeName);
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

bool plStateMachineState_Compound::GetInstanceDataDesc(plInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubStates.GetArrayPtr(), out_desc);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plStateMachineLogicOperator, 1)
  PL_ENUM_CONSTANTS(plStateMachineLogicOperator::And, plStateMachineLogicOperator::Or)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineTransition_BlackboardConditions, 1, plRTTIDefaultAllocator<plStateMachineTransition_BlackboardConditions>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Operator", plStateMachineLogicOperator, m_Operator),
    PL_ARRAY_MEMBER_PROPERTY("Conditions", m_Conditions),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineTransition_BlackboardConditions::plStateMachineTransition_BlackboardConditions() = default;
plStateMachineTransition_BlackboardConditions::~plStateMachineTransition_BlackboardConditions() = default;

bool plStateMachineTransition_BlackboardConditions::IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const
{
  if (m_Conditions.IsEmpty())
    return true;

  auto pBlackboard = ref_instance.GetBlackboard();
  if (pBlackboard == nullptr)
    return false;

  const bool bCheckFor = (m_Operator == plStateMachineLogicOperator::Or) ? true : false;
  for (auto& condition : m_Conditions)
  {
    if (condition.IsConditionMet(*pBlackboard) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

plResult plStateMachineTransition_BlackboardConditions::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;
  return inout_stream.WriteArray(m_Conditions);
}

plResult plStateMachineTransition_BlackboardConditions::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Operator;
  return inout_stream.ReadArray(m_Conditions);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineTransition_Timeout, 1, plRTTIDefaultAllocator<plStateMachineTransition_Timeout>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Timeout", m_Timeout),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineTransition_Timeout::plStateMachineTransition_Timeout() = default;
plStateMachineTransition_Timeout::~plStateMachineTransition_Timeout() = default;

bool plStateMachineTransition_Timeout::IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetTimeInCurrentState() >= m_Timeout;
}

plResult plStateMachineTransition_Timeout::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Timeout;
  return PL_SUCCESS;
}

plResult plStateMachineTransition_Timeout::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_Timeout;
  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineTransition_Compound, 1, plRTTIDefaultAllocator<plStateMachineTransition_Compound>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Operator", plStateMachineLogicOperator, m_Operator),
    PL_ARRAY_MEMBER_PROPERTY("SubTransitions", m_SubTransitions)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineTransition_Compound::plStateMachineTransition_Compound() = default;

plStateMachineTransition_Compound::~plStateMachineTransition_Compound()
{
  for (auto pSubState : m_SubTransitions)
  {
    auto pAllocator = pSubState->GetDynamicRTTI()->GetAllocator();
    pAllocator->Deallocate(pSubState);
  }
}

bool plStateMachineTransition_Compound::IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const
{
  auto pData = static_cast<plStateMachineInternal::Compound::InstanceData*>(pInstanceData);
  m_Compound.Initialize(pData);

  const bool bCheckFor = (m_Operator == plStateMachineLogicOperator::Or) ? true : false;
  for (plUInt32 i = 0; i < m_SubTransitions.GetCount(); ++i)
  {
    void* pSubInstanceData = m_Compound.GetSubInstanceData(pData, i);
    if (m_SubTransitions[i]->IsConditionMet(ref_instance, pSubInstanceData) == bCheckFor)
      return bCheckFor;
  }

  return !bCheckFor;
}

plResult plStateMachineTransition_Compound::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_Operator;

  const plUInt32 uiNumSubTransitions = m_SubTransitions.GetCount();
  inout_stream << uiNumSubTransitions;

  for (auto pSubTransition : m_SubTransitions)
  {
    auto pStateType = pSubTransition->GetDynamicRTTI();
    plTypeVersionWriteContext::GetContext()->AddType(pStateType);

    inout_stream << pStateType->GetTypeName();
    PL_SUCCEED_OR_RETURN(pSubTransition->Serialize(inout_stream));
  }

  return PL_SUCCESS;
}

plResult plStateMachineTransition_Compound::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_Operator;

  plUInt32 uiNumSubTransitions = 0;
  inout_stream >> uiNumSubTransitions;
  m_SubTransitions.Reserve(uiNumSubTransitions);

  plStringBuilder sTypeName;
  for (plUInt32 i = 0; i < uiNumSubTransitions; ++i)
  {
    inout_stream >> sTypeName;
    if (const plRTTI* pType = plRTTI::FindTypeByName(sTypeName))
    {
      plUniquePtr<plStateMachineTransition> pSubTransition = pType->GetAllocator()->Allocate<plStateMachineTransition>();
      PL_SUCCEED_OR_RETURN(pSubTransition->Deserialize(inout_stream));

      m_SubTransitions.PushBack(pSubTransition.Release());
    }
    else
    {
      plLog::Error("Unknown state machine state type '{}'", sTypeName);
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

bool plStateMachineTransition_Compound::GetInstanceDataDesc(plInstanceDataDesc& out_desc)
{
  return m_Compound.GetInstanceDataDesc(m_SubTransitions.GetArrayPtr(), out_desc);
}


//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineTransition_TransitionEvent, 1, plRTTIDefaultAllocator<plStateMachineTransition_TransitionEvent>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("EventName", m_sEventName),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineTransition_TransitionEvent::plStateMachineTransition_TransitionEvent() = default;
plStateMachineTransition_TransitionEvent::~plStateMachineTransition_TransitionEvent() = default;

bool plStateMachineTransition_TransitionEvent::IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const
{
  return ref_instance.GetCurrentTransitionEvent() == m_sEventName;
}

plResult plStateMachineTransition_TransitionEvent::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));

  inout_stream << m_sEventName;
  return PL_SUCCESS;
}

plResult plStateMachineTransition_TransitionEvent::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));

  inout_stream >> m_sEventName;
  return PL_SUCCESS;
}


PL_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineBuiltins);
