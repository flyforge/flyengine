#pragma once

#include <Core/Utils/Blackboard.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

/// \brief A state machine state implementation that represents another state machine nested within this state. This can be used to build hierarchical state machines.
class PLASMA_GAMEENGINE_DLL plStateMachineState_NestedStateMachine : public plStateMachineState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineState_NestedStateMachine, plStateMachineState);

public:
  plStateMachineState_NestedStateMachine(plStringView sName = plStringView());
  ~plStateMachineState_NestedStateMachine();

  virtual void OnEnter(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pFromState) const override;
  virtual void OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const override;
  virtual void Update(plStateMachineInstance& ref_instance, void* pInstanceData, plTime deltaTime) const override;

  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) override;

  void SetResource(const plStateMachineResourceHandle& hResource);
  const plStateMachineResourceHandle& GetResource() const { return m_hResource; }

  void SetResourceFile(const char* szFile); // [ property ]
  const char* GetResourceFile() const;      // [ property ]

  /// \brief Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

private:
  plStateMachineResourceHandle m_hResource;
  plHashedString m_sInitialState;

  // Should the inner state machine keep its current state on exit and re-enter or should it exit as well and re-enter the initial state again.
  bool m_bKeepCurrentStateOnExit = false;

  struct InstanceData
  {
    plUniquePtr<plStateMachineInstance> m_pStateMachineInstance;
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine state implementation that combines multiple sub states into one.
///
/// Can be used to build states in a more modular way. All calls are simply redirected to all sub states,
/// e.g. when entered it calls OnEnter on all its sub states.
class PLASMA_GAMEENGINE_DLL plStateMachineState_Compound : public plStateMachineState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineState_Compound, plStateMachineState);

public:
  plStateMachineState_Compound(plStringView sName = plStringView());
  ~plStateMachineState_Compound();

  virtual void OnEnter(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pFromState) const override;
  virtual void OnExit(plStateMachineInstance& ref_instance, void* pInstanceData, const plStateMachineState* pToState) const override;
  virtual void Update(plStateMachineInstance& ref_instance, void* pInstanceData, plTime deltaTime) const override;

  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) override;

  plSmallArray<plStateMachineState*, 2> m_SubStates;

private:
  plStateMachineInternal::Compound m_Compound;
};

//////////////////////////////////////////////////////////////////////////

/// \brief An enum that represents the operator of a comparison
struct PLASMA_GAMEENGINE_DLL plStateMachineLogicOperator
{
  using StorageType = plUInt8;

  enum Enum
  {
    And,
    Or,

    Default = And
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plStateMachineLogicOperator);

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that checks the instance's blackboard for the given conditions.
class PLASMA_GAMEENGINE_DLL plStateMachineTransition_BlackboardConditions : public plStateMachineTransition
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineTransition_BlackboardConditions, plStateMachineTransition);

public:
  plStateMachineTransition_BlackboardConditions();
  ~plStateMachineTransition_BlackboardConditions();

  virtual bool IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  plEnum<plStateMachineLogicOperator> m_Operator;
  plHybridArray<plBlackboardCondition, 2> m_Conditions;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that triggers after the given time
class PLASMA_GAMEENGINE_DLL plStateMachineTransition_Timeout : public plStateMachineTransition
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineTransition_Timeout, plStateMachineTransition);

public:
  plStateMachineTransition_Timeout();
  ~plStateMachineTransition_Timeout();

  virtual bool IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  plTime m_Timeout;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine transition implementation that combines multiple sub transition into one.
///
/// Can be used to build transitions in a more modular way. All calls are simply redirected to all sub transitions
/// and then combined with the given logic operator (AND, OR).
class PLASMA_GAMEENGINE_DLL plStateMachineTransition_Compound : public plStateMachineTransition
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineTransition_Compound, plStateMachineTransition);

public:
  plStateMachineTransition_Compound();
  ~plStateMachineTransition_Compound();

  virtual bool IsConditionMet(plStateMachineInstance& ref_instance, void* pInstanceData) const override;

  virtual plResult Serialize(plStreamWriter& inout_stream) const override;
  virtual plResult Deserialize(plStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc) override;

  plEnum<plStateMachineLogicOperator> m_Operator;
  plSmallArray<plStateMachineTransition*, 2> m_SubTransitions;

private:
  plStateMachineInternal::Compound m_Compound;
};
