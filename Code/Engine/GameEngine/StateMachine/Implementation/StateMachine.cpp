#include <GameEngine/GameEnginePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Utils/Blackboard.h>
#include <Core/World/Component.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineState, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(GetName),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_OnEnter, In, "StateMachineInstance", In, "FromState")->AddAttributes(new plScriptBaseClassFunctionAttribute(plStateMachineState_ScriptBaseClassFunctions::OnEnter)),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_OnExit, In, "StateMachineInstance", In, "ToState")->AddAttributes(new plScriptBaseClassFunctionAttribute(plStateMachineState_ScriptBaseClassFunctions::OnExit)),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_Update, In, "StateMachineInstance", In, "DeltaTime")->AddAttributes(new plScriptBaseClassFunctionAttribute(plStateMachineState_ScriptBaseClassFunctions::Update)),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineState_Empty, 1, plRTTIDefaultAllocator<plStateMachineState_Empty>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plHiddenAttribute(),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineState::plStateMachineState(plStringView sName)
{
  m_sName.Assign(sName);
}

void plStateMachineState::SetName(plStringView sName)
{
  PL_ASSERT_DEV(m_sName.IsEmpty(), "Name can't be changed afterwards");
  m_sName.Assign(sName);
}

void plStateMachineState::OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const
{
}

void plStateMachineState::Update(plStateMachineInstance& ref_instance, void* pInstanceData, plTime deltaTime) const
{
}

plResult plStateMachineState::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_sName;
  return PL_SUCCESS;
}

plResult plStateMachineState::Deserialize(plStreamReader& inout_stream)
{
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);

  inout_stream >> m_sName;
  return PL_SUCCESS;
}

bool plStateMachineState::GetInstanceDataDesc(plInstanceDataDesc& out_desc)
{
  return false;
}

void plStateMachineState::Reflection_OnEnter(plStateMachineInstance* pStateMachineInstance, const plStateMachineState* pFromState)
{
}

void plStateMachineState::Reflection_OnExit(plStateMachineInstance* pStateMachineInstance, const plStateMachineState* pToState)
{
}

void plStateMachineState::Reflection_Update(plStateMachineInstance* pStateMachineInstance, plTime deltaTime)
{
}

plStateMachineState_Empty::plStateMachineState_Empty(plStringView sName)
  : plStateMachineState(sName)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineTransition, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plStateMachineTransition::Serialize(plStreamWriter& inout_stream) const
{
  return PL_SUCCESS;
}

plResult plStateMachineTransition::Deserialize(plStreamReader& inout_stream)
{
  return PL_SUCCESS;
}

bool plStateMachineTransition::GetInstanceDataDesc(plInstanceDataDesc& out_desc)
{
  return false;
}

//////////////////////////////////////////////////////////////////////////

plStateMachineDescription::plStateMachineDescription() = default;
plStateMachineDescription::~plStateMachineDescription() = default;

plUInt32 plStateMachineDescription::AddState(plUniquePtr<plStateMachineState>&& pState)
{
  const plUInt32 uiIndex = m_States.GetCount();

  auto& sStateName = pState->GetNameHashed();
  if (sStateName.IsEmpty() == false)
  {
    PL_VERIFY(m_StateNameToIndexTable.Contains(sStateName) == false, "A state with name '{}' already exists.", sStateName);
    m_StateNameToIndexTable.Insert(sStateName, uiIndex);
  }

  StateContext& stateContext = m_States.ExpandAndGetRef();

  plInstanceDataDesc instanceDataDesc;
  if (pState->GetInstanceDataDesc(instanceDataDesc))
  {
    stateContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  stateContext.m_pState = std::move(pState);

  return uiIndex;
}

void plStateMachineDescription::AddTransition(plUInt32 uiFromStateIndex, plUInt32 uiToStateIndex, plUniquePtr<plStateMachineTransition>&& pTransistion)
{
  PL_ASSERT_DEV(uiFromStateIndex != uiToStateIndex, "Can't add a transition to itself");

  TransitionArray* pTransitions = nullptr;
  if (uiFromStateIndex == plInvalidIndex)
  {
    pTransitions = &m_FromAnyTransitions;
  }
  else
  {
    PL_ASSERT_DEV(uiFromStateIndex < m_States.GetCount(), "Invalid from state index {}", uiFromStateIndex);
    pTransitions = &m_States[uiFromStateIndex].m_Transitions;
  }

  PL_ASSERT_DEV(uiToStateIndex < m_States.GetCount(), "Invalid to state index {}", uiToStateIndex);

  TransitionContext& transitionContext = pTransitions->ExpandAndGetRef();

  plInstanceDataDesc instanceDataDesc;
  if (pTransistion->GetInstanceDataDesc(instanceDataDesc))
  {
    transitionContext.m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(instanceDataDesc);
  }

  transitionContext.m_pTransition = std::move(pTransistion);
  transitionContext.m_uiToStateIndex = uiToStateIndex;
}

constexpr plTypeVersion s_StateMachineDescriptionVersion = 1;

plResult plStateMachineDescription::Serialize(plStreamWriter& ref_originalStream) const
{
  ref_originalStream.WriteVersion(s_StateMachineDescriptionVersion);

  plStringDeduplicationWriteContext stringDeduplicationWriteContext(ref_originalStream);
  plTypeVersionWriteContext typeVersionWriteContext;
  auto& stream = typeVersionWriteContext.Begin(stringDeduplicationWriteContext.Begin());

  plUInt32 uiNumTransitions = m_FromAnyTransitions.GetCount();

  // states
  {
    const plUInt32 uiNumStates = m_States.GetCount();
    stream << uiNumStates;

    for (auto& stateContext : m_States)
    {
      auto pStateType = stateContext.m_pState->GetDynamicRTTI();
      typeVersionWriteContext.AddType(pStateType);

      stream << pStateType->GetTypeName();
      PL_SUCCEED_OR_RETURN(stateContext.m_pState->Serialize(stream));

      uiNumTransitions += stateContext.m_Transitions.GetCount();
    }
  }

  // transitions
  {
    stream << uiNumTransitions;

    auto SerializeTransitions = [&](const TransitionArray& transitions, plUInt32 uiFromStateIndex) -> plResult {
      for (auto& transitionContext : transitions)
      {
        const plUInt32 uiToStateIndex = transitionContext.m_uiToStateIndex;

        stream << uiFromStateIndex;
        stream << uiToStateIndex;

        auto pTransitionType = transitionContext.m_pTransition->GetDynamicRTTI();
        typeVersionWriteContext.AddType(pTransitionType);

        stream << pTransitionType->GetTypeName();
        PL_SUCCEED_OR_RETURN(transitionContext.m_pTransition->Serialize(stream));
      }

      return PL_SUCCESS;
    };

    PL_SUCCEED_OR_RETURN(SerializeTransitions(m_FromAnyTransitions, plInvalidIndex));

    for (plUInt32 uiFromStateIndex = 0; uiFromStateIndex < m_States.GetCount(); ++uiFromStateIndex)
    {
      auto& transitions = m_States[uiFromStateIndex].m_Transitions;

      PL_SUCCEED_OR_RETURN(SerializeTransitions(transitions, uiFromStateIndex));
    }
  }

  PL_SUCCEED_OR_RETURN(typeVersionWriteContext.End());
  PL_SUCCEED_OR_RETURN(stringDeduplicationWriteContext.End());

  return PL_SUCCESS;
}

plResult plStateMachineDescription::Deserialize(plStreamReader& inout_stream)
{
  const auto uiVersion = inout_stream.ReadVersion(s_StateMachineDescriptionVersion);
  PL_IGNORE_UNUSED(uiVersion);

  plStringDeduplicationReadContext stringDeduplicationReadContext(inout_stream);
  plTypeVersionReadContext typeVersionReadContext(inout_stream);

  plStringBuilder sTypeName;

  // states
  {
    plUInt32 uiNumStates = 0;
    inout_stream >> uiNumStates;

    for (plUInt32 i = 0; i < uiNumStates; ++i)
    {
      inout_stream >> sTypeName;
      if (const plRTTI* pType = plRTTI::FindTypeByName(sTypeName))
      {
        plUniquePtr<plStateMachineState> pState = pType->GetAllocator()->Allocate<plStateMachineState>();
        PL_SUCCEED_OR_RETURN(pState->Deserialize(inout_stream));

        PL_VERIFY(AddState(std::move(pState)) == i, "Implementation error");
      }
      else
      {
        plLog::Error("Unknown state machine state type '{}'", sTypeName);
        return PL_FAILURE;
      }
    }
  }

  // transitions
  {
    plUInt32 uiNumTransitions = 0;
    inout_stream >> uiNumTransitions;

    for (plUInt32 i = 0; i < uiNumTransitions; ++i)
    {
      plUInt32 uiFromStateIndex = 0;
      plUInt32 uiToStateIndex = 0;

      inout_stream >> uiFromStateIndex;
      inout_stream >> uiToStateIndex;

      inout_stream >> sTypeName;
      if (const plRTTI* pType = plRTTI::FindTypeByName(sTypeName))
      {
        plUniquePtr<plStateMachineTransition> pTransition = pType->GetAllocator()->Allocate<plStateMachineTransition>();
        PL_SUCCEED_OR_RETURN(pTransition->Deserialize(inout_stream));

        AddTransition(uiFromStateIndex, uiToStateIndex, std::move(pTransition));
      }
      else
      {
        plLog::Error("Unknown state machine transition type '{}'", sTypeName);
        return PL_FAILURE;
      }
    }
  }

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plStateMachineInstance, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_SetState, In, "StateName"),
    PL_SCRIPT_FUNCTION_PROPERTY(GetCurrentState),
    PL_SCRIPT_FUNCTION_PROPERTY(GetTimeInCurrentState),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOwnerComponent),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_GetBlackboard),
  }
  PL_END_FUNCTIONS;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plStateMachineInstance::plStateMachineInstance(plReflectedClass& ref_owner, const plSharedPtr<const plStateMachineDescription>& pDescription /*= nullptr*/)
  : m_Owner(ref_owner)
  , m_pDescription(pDescription)
{
  if (pDescription != nullptr)
  {
    m_InstanceData = pDescription->m_InstanceDataAllocator.AllocateAndConstruct();
  }
}

plStateMachineInstance::~plStateMachineInstance()
{
  ExitCurrentState(nullptr);

  m_pCurrentState = nullptr;
  m_uiCurrentStateIndex = plInvalidIndex;

  if (m_pDescription != nullptr)
  {
    m_pDescription->m_InstanceDataAllocator.DestructAndDeallocate(m_InstanceData);
  }
}

plResult plStateMachineInstance::SetState(plStateMachineState* pState)
{
  if (m_pCurrentState == pState)
    return PL_SUCCESS;

  if (pState != nullptr && m_pDescription != nullptr)
  {
    return SetState(pState->GetNameHashed());
  }

  const auto pFromState = m_pCurrentState;
  const auto pToState = pState;

  ExitCurrentState(pToState);

  m_pCurrentState = pState;
  m_uiCurrentStateIndex = plInvalidIndex;
  m_pCurrentTransitions = nullptr;

  EnterCurrentState(pFromState);

  return PL_SUCCESS;
}

plResult plStateMachineInstance::SetState(const plHashedString& sStateName)
{
  PL_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by name");

  plUInt32 uiStateIndex = 0;
  if (m_pDescription->m_StateNameToIndexTable.TryGetValue(sStateName, uiStateIndex))
  {
    SetStateInternal(uiStateIndex);
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plStateMachineInstance::SetState(plUInt32 uiStateIndex)
{
  PL_ASSERT_DEV(m_pDescription != nullptr, "Must have a description to set state by index");

  if (uiStateIndex < m_pDescription->m_States.GetCount())
  {
    SetStateInternal(uiStateIndex);
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plStateMachineInstance::SetStateOrFallback(const plHashedString& sStateName, plUInt32 uiFallbackStateIndex /*= 0*/)
{
  if (SetState(sStateName).Failed())
  {
    return SetState(uiFallbackStateIndex);
  }

  return PL_SUCCESS;
}

void plStateMachineInstance::Update(plTime deltaTime)
{
  plUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != plInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->Update(*this, pInstanceData, deltaTime);
  }

  m_TimeInCurrentState += deltaTime;
}

plWorld* plStateMachineInstance::GetOwnerWorld()
{
  if (auto pComponent = plDynamicCast<plComponent*>(&m_Owner))
  {
    return pComponent->GetWorld();
  }

  return nullptr;
}

void plStateMachineInstance::SetBlackboard(const plSharedPtr<plBlackboard>& pBlackboard)
{
  m_pBlackboard = pBlackboard;
}

void plStateMachineInstance::FireTransitionEvent(plStringView sEvent)
{
  m_sCurrentTransitionEvent = sEvent;

  plUInt32 uiNewStateIndex = FindNewStateToTransitionTo();
  if (uiNewStateIndex != plInvalidIndex)
  {
    SetState(uiNewStateIndex).IgnoreResult();
  }

  m_sCurrentTransitionEvent = {};
}

bool plStateMachineInstance::Reflection_SetState(const plHashedString& sStateName)
{
  return SetState(sStateName).Succeeded();
}

plComponent* plStateMachineInstance::Reflection_GetOwnerComponent() const
{
  return plDynamicCast<plComponent*>(&m_Owner);
}

void plStateMachineInstance::SetStateInternal(plUInt32 uiStateIndex)
{
  if (m_uiCurrentStateIndex == uiStateIndex)
    return;

  const auto& stateContext = m_pDescription->m_States[uiStateIndex];
  const auto pFromState = m_pCurrentState;
  const auto pToState = stateContext.m_pState.Borrow();

  ExitCurrentState(pToState);

  m_pCurrentState = pToState;
  m_uiCurrentStateIndex = uiStateIndex;
  m_pCurrentTransitions = &stateContext.m_Transitions;

  EnterCurrentState(pFromState);
}

void plStateMachineInstance::EnterCurrentState(const plStateMachineState* pFromState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnEnter(*this, pInstanceData, pFromState);

    m_TimeInCurrentState = plTime::MakeZero();
  }
}

void plStateMachineInstance::ExitCurrentState(const plStateMachineState* pToState)
{
  if (m_pCurrentState != nullptr)
  {
    void* pInstanceData = GetCurrentStateInstanceData();
    m_pCurrentState->OnExit(*this, pInstanceData, pToState);
  }
}

plUInt32 plStateMachineInstance::FindNewStateToTransitionTo()
{
  if (m_pCurrentTransitions != nullptr)
  {
    for (auto& transitionContext : *m_pCurrentTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  if (m_pDescription != nullptr)
  {
    for (auto& transitionContext : m_pDescription->m_FromAnyTransitions)
    {
      void* pInstanceData = GetInstanceData(transitionContext.m_uiInstanceDataOffset);
      if (transitionContext.m_pTransition->IsConditionMet(*this, pInstanceData))
      {
        return transitionContext.m_uiToStateIndex;
      }
    }
  }

  return plInvalidIndex;
}


PL_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachine);
