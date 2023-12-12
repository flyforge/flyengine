#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <GameEngine/StateMachine/StateMachineBuiltins.h>

namespace
{
  class TestState : public plStateMachineState
  {
    PLASMA_ADD_DYNAMIC_REFLECTION(TestState, plStateMachineState);

  public:
    TestState(plStringView sName = plStringView())
      : plStateMachineState(sName)
    {
    }

    virtual void OnEnter(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pFromState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiEnterCounter++;

      m_CounterTable[&ref_instance] = pData->m_Counter;
    }

    virtual void OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiExitCounter++;

      m_CounterTable[&ref_instance] = pData->m_Counter;
    }

    virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct Counter
    {
      plUInt32 m_uiEnterCounter = 0;
      plUInt32 m_uiExitCounter = 0;
    };

    mutable plHashTable<plStateMachineInstance*, Counter> m_CounterTable;

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      Counter m_Counter;

      static plUInt32 s_uiConstructionCounter;
      static plUInt32 s_uiDestructionCounter;
    };
  };

  plUInt32 TestState::InstanceData::s_uiConstructionCounter = 0;
  plUInt32 TestState::InstanceData::s_uiDestructionCounter = 0;

  // clang-format off
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(TestState, 1, plRTTIDefaultAllocator<TestState>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class TestTransition : public plStateMachineTransition
  {
  public:
    bool IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_uiConditionCounter++;

      return pData->m_uiConditionCounter > 1;
    }

    bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      plUInt32 m_uiConditionCounter;

      static plUInt32 s_uiConstructionCounter;
      static plUInt32 s_uiDestructionCounter;
    };
  };

  plUInt32 TestTransition::InstanceData::s_uiConstructionCounter = 0;
  plUInt32 TestTransition::InstanceData::s_uiDestructionCounter = 0;

  static void ResetCounter()
  {
    TestState::InstanceData::s_uiConstructionCounter = 0;
    TestState::InstanceData::s_uiDestructionCounter = 0;
    TestTransition::InstanceData::s_uiConstructionCounter = 0;
    TestTransition::InstanceData::s_uiDestructionCounter = 0;
  }

  static plTime s_TimeStep = plTime::Milliseconds(10);

} // namespace

void plGameEngineTestStateMachine::RunBuiltinsTest()
{
  plReflectedClass fakeOwner;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple States")
  {
    ResetCounter();

    plSharedPtr<plStateMachineDescription> pDesc = PLASMA_DEFAULT_NEW(plStateMachineDescription);

    auto pStateA = PLASMA_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = PLASMA_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition = PLASMA_DEFAULT_NEW(TestTransition);
    pDesc->AddTransition(1, 0, pTransition);

    plStateMachineInstance* pInstance = nullptr;
    {
      plStateMachineInstance sm(fakeOwner, pDesc);
      PLASMA_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 2);
      PLASMA_TEST_INT(TestTransition::InstanceData::s_uiConstructionCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      plHashedString sStateName; // intentionally left empty to go to fallback state (state with index 0 -> state "A")
      PLASMA_TEST_BOOL(sm.SetStateOrFallback(sStateName).Succeeded());
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      PLASMA_TEST_BOOL(sm.SetState(pStateB).Succeeded());
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sStateName.Assign("C");
      PLASMA_TEST_BOOL(sm.SetState(sStateName).Failed());

      // no transition yet
      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // go back to "A"
      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 2);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);

      pInstance = &sm; // will be dead after this line but we only need the pointer
    }

    PLASMA_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 2);
    PLASMA_TEST_INT(TestTransition::InstanceData::s_uiDestructionCounter, 1);
    PLASMA_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiEnterCounter, 2);
    PLASMA_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiExitCounter, 2);
    PLASMA_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiEnterCounter, 1);
    PLASMA_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiExitCounter, 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Blackboard Transition")
  {
    ResetCounter();

    plSharedPtr<plStateMachineDescription> pDesc = PLASMA_DEFAULT_NEW(plStateMachineDescription);

    auto pStateA = PLASMA_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = PLASMA_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pStateC = PLASMA_DEFAULT_NEW(TestState, "C");
    pDesc->AddState(pStateC);

    plHashedString sTestVal = plMakeHashedString("TestVal");
    plHashedString sTestVal2 = plMakeHashedString("TestVal2");

    {
      auto pTransition = PLASMA_DEFAULT_NEW(plStateMachineTransition_BlackboardConditions);
      auto& cond = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName = sTestVal;
      cond.m_fComparisonValue = 2;
      cond.m_Operator = plComparisonOperator::Greater;

      auto& cond2 = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName = sTestVal2;
      cond2.m_fComparisonValue = 10;
      cond2.m_Operator = plComparisonOperator::Equal;

      pDesc->AddTransition(0, 1, pTransition);
    }

    {
      auto pTransition = PLASMA_DEFAULT_NEW(plStateMachineTransition_BlackboardConditions);
      pTransition->m_Operator = plStateMachineLogicOperator::Or;

      auto& cond = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName = sTestVal;
      cond.m_fComparisonValue = 3;
      cond.m_Operator = plComparisonOperator::Greater;

      auto& cond2 = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName = sTestVal2;
      cond2.m_fComparisonValue = 20;
      cond2.m_Operator = plComparisonOperator::Equal;

      pDesc->AddTransition(1, 2, pTransition);
    }

    {
      plSharedPtr<plBlackboard> pBlackboard = plBlackboard::Create();
      pBlackboard->RegisterEntry(sTestVal, 2);
      pBlackboard->RegisterEntry(sTestVal2, 0);

      plStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      PLASMA_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      // no transition yet since only part of the conditions is true
      PLASMA_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal, 3).Succeeded());
      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      PLASMA_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to B
      PLASMA_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal2, 10).Succeeded());
      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      PLASMA_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to C, only part of the condition needed because of 'OR' operator
      PLASMA_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal2, 20).Succeeded());
      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Timeout Transition")
  {
    ResetCounter();

    plSharedPtr<plStateMachineDescription> pDesc = PLASMA_DEFAULT_NEW(plStateMachineDescription);

    auto pStateA = PLASMA_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = PLASMA_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition = PLASMA_DEFAULT_NEW(plStateMachineTransition_Timeout);
    pTransition->m_Timeout = plTime::Milliseconds(5);
    pDesc->AddTransition(0, 1, pTransition);

    {
      plStateMachineInstance sm(fakeOwner, pDesc);
      PLASMA_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compounds")
  {
    ResetCounter();

    plSharedPtr<plStateMachineDescription> pDesc = PLASMA_DEFAULT_NEW(plStateMachineDescription);

    auto pCompoundState = PLASMA_DEFAULT_NEW(plStateMachineState_Compound, "A");
    {
      auto pAllocator = plGetStaticRTTI<TestState>()->GetAllocator();
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
    }
    pDesc->AddState(pCompoundState);

    auto pStateB = PLASMA_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    plHashedString sTestVal = plMakeHashedString("TestVal");

    {
      auto pCompoundTransition = PLASMA_DEFAULT_NEW(plStateMachineTransition_Compound);

      {
        auto pAllocator = plGetStaticRTTI<plStateMachineTransition_BlackboardConditions>()->GetAllocator();
        auto pSubTransition = pAllocator->Allocate<plStateMachineTransition_BlackboardConditions>();

        auto& cond = pSubTransition->m_Conditions.ExpandAndGetRef();
        cond.m_sEntryName = sTestVal;
        cond.m_fComparisonValue = 2;
        cond.m_Operator = plComparisonOperator::Greater;

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      {
        auto pAllocator = plGetStaticRTTI<plStateMachineTransition_Timeout>()->GetAllocator();
        auto pSubTransition = pAllocator->Allocate<plStateMachineTransition_Timeout>();
        pSubTransition->m_Timeout = plTime::Milliseconds(5);

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      pDesc->AddTransition(0, 1, pCompoundTransition);
    }

    {
      plSharedPtr<plBlackboard> pBlackboard = plBlackboard::Create();
      pBlackboard->RegisterEntry(sTestVal, 2);

      plStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      PLASMA_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 1); // Compound instance data not constructed yet

      PLASMA_TEST_BOOL(sm.SetState(pCompoundState).Succeeded());
      PLASMA_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 3);
      PLASMA_TEST_INT(plStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(plStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // no transition yet because timeout is not reached yet
      PLASMA_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal, 3).Succeeded());
      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // all conditions met, transition to B
      sm.Update(s_TimeStep);
      PLASMA_TEST_INT(plStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(plStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiExitCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      PLASMA_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }

    PLASMA_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 3);
  }
}
