#pragma once

#include <GameEngine/StateMachine/Implementation/StateMachineInstanceData.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

class plComponent;
class plWorld;
class plBlackboard;
class plStateMachineInstance;

/// \brief Base class for a state in a state machine.
///
/// Note that states are shared between multiple instances and thus
/// shouldn't modify any data on their own but always operate on the passed instance and instance data.
/// \see plStateMachineInstanceDataDesc
class PLASMA_GAMEENGINE_DLL plStateMachineState : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineState, plReflectedClass);

public:
  plStateMachineState(plStringView sName = plStringView());

  void SetName(plStringView sName);
  plStringView GetName() const { return m_sName; }
  const plHashedString& GetNameHashed() const { return m_sName; }

  virtual void OnEnter(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pFromState) const = 0;
  virtual void OnExit(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pToState) const;
  virtual void Update(plStateMachineInstance& instance, void* pInstanceData, plTime deltaTime) const;

  virtual plResult Serialize(plStreamWriter& stream) const;
  virtual plResult Deserialize(plStreamReader& stream);

  /// \brief Returns whether this state needs additional instance data and if so fills the out_desc.
  ///
  /// \see plStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc);

private:
  // These are dummy functions for the scripting reflection
  void Reflection_OnEnter(plStateMachineInstance* pStateMachineInstance, const plStateMachineState* pFromState);
  void Reflection_OnExit(plStateMachineInstance* pStateMachineInstance, const plStateMachineState* pToState);
  void Reflection_Update(plStateMachineInstance* pStateMachineInstance, plTime deltaTime);

  plHashedString m_sName;
};

struct plStateMachineState_ScriptBaseClassFunctions
{
  enum Enum
  {
    OnEnter,
    OnExit,
    Update,

    Count
  };
};

/// \brief Base class for a transition in a state machine. The target state of a transition is automatically set
/// once its condition has been met.
///
/// Same as with states, transitions are also shared between multiple instances and thus
/// should decide their condition based on the passed instance and instance data.
/// \see plStateMachineInstanceDataDesc
class PLASMA_GAMEENGINE_DLL plStateMachineTransition : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineTransition, plReflectedClass);

  virtual bool IsConditionMet(plStateMachineInstance& instance, void* pInstanceData) const = 0;

  virtual plResult Serialize(plStreamWriter& stream) const;
  virtual plResult Deserialize(plStreamReader& stream);

  /// \brief Returns whether this transition needs additional instance data and if so fills the out_desc.
  ///
  /// \see plStateMachineInstanceDataDesc
  virtual bool GetInstanceDataDesc(plInstanceDataDesc& out_desc);
};

/// \brief The state machine description defines the structure of a state machine like e.g.
/// what states it has and how to transition between them.
/// Once an instance is created from a description it is not allowed to change the description afterwards.
class PLASMA_GAMEENGINE_DLL plStateMachineDescription : public plRefCounted
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plStateMachineDescription);

public:
  plStateMachineDescription();
  ~plStateMachineDescription();

  /// \brief Adds the given state to the description and returns the state index.
  plUInt32 AddState(plUniquePtr<plStateMachineState>&& pState);

  /// \brief Adds the given transition between the two given states. A uiFromStateIndex of plInvalidIndex generates a transition that can be done from any other possible state.
  void AddTransition(plUInt32 uiFromStateIndex, plUInt32 uiToStateIndex, plUniquePtr<plStateMachineTransition>&& pTransistion);

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);

private:
  friend class plStateMachineInstance;

  struct TransitionContext
  {
    plUniquePtr<plStateMachineTransition> m_pTransition;
    plUInt32 m_uiToStateIndex = 0;
    plUInt32 m_uiInstanceDataOffset = plInvalidIndex;
  };

  using TransitionArray = plSmallArray<TransitionContext, 2>;
  TransitionArray m_FromAnyTransitions;

  struct StateContext
  {
    plUniquePtr<plStateMachineState> m_pState;
    TransitionArray m_Transitions;
    plUInt32 m_uiInstanceDataOffset = plInvalidIndex;
  };

  plDynamicArray<StateContext> m_States;
  plHashTable<plHashedString, plUInt32> m_StateNameToIndexTable;

  plInstanceDataAllocator m_InstanceDataAllocator;
};

/// \brief The state machine instance represents the actual state machine.
/// Typically it is created from a description but for small use cases it can also be used without a description.
class PLASMA_GAMEENGINE_DLL plStateMachineInstance
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plStateMachineInstance);
  
public:
  plStateMachineInstance(plReflectedClass& ref_owner, const plSharedPtr<const plStateMachineDescription>& pDescription = nullptr);
  ~plStateMachineInstance();

  plResult SetState(plStateMachineState* pState);
  plResult SetState(plUInt32 uiStateIndex);
  plResult SetState(const plHashedString& sStateName);
  plResult SetStateOrFallback(const plHashedString& sStateName, plUInt32 uiFallbackStateIndex = 0);
  plStateMachineState* GetCurrentState() { return m_pCurrentState; }

  void Update(plTime deltaTime);

  plReflectedClass& GetOwner() { return m_Owner; }
  plWorld* GetOwnerWorld();

  void SetBlackboard(const plSharedPtr<plBlackboard>& pBlackboard);
  const plSharedPtr<plBlackboard>& GetBlackboard() const { return m_pBlackboard; }

  /// \brief Returns how long the state machine is in its current state
  plTime GetTimeInCurrentState() const { return m_TimeInCurrentState; }

  /// \brief Sends a named event that state transitions can react to.
  void FireTransitionEvent(plStringView sEvent);

  plStringView GetCurrentTransitionEvent() const { return m_sCurrentTransitionEvent; }

private:
  PLASMA_ALLOW_PRIVATE_PROPERTIES(plStateMachineInstance);

  bool Reflection_SetState(const plHashedString& sStateName);
  plComponent* Reflection_GetOwnerComponent() const;
  plBlackboard* Reflection_GetBlackboard() const { return m_pBlackboard.Borrow(); }

  void SetStateInternal(plUInt32 uiStateIndex);
  void EnterCurrentState(const plStateMachineState* pFromState);
  void ExitCurrentState(const plStateMachineState* pToState);
  plUInt32 FindNewStateToTransitionTo();

  PLASMA_ALWAYS_INLINE void* GetInstanceData(plUInt32 uiOffset)
  {
    return plInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), uiOffset);
  }

  PLASMA_ALWAYS_INLINE void* GetCurrentStateInstanceData()
  {
    if (m_pDescription != nullptr && m_uiCurrentStateIndex < m_pDescription->m_States.GetCount())
    {
      return GetInstanceData(m_pDescription->m_States[m_uiCurrentStateIndex].m_uiInstanceDataOffset);
    }
    return nullptr;
  }

  plReflectedClass& m_Owner;
  plSharedPtr<const plStateMachineDescription> m_pDescription;
  plSharedPtr<plBlackboard> m_pBlackboard;

  plStateMachineState* m_pCurrentState = nullptr;
  plUInt32 m_uiCurrentStateIndex = plInvalidIndex;
  plTime m_TimeInCurrentState;
  plStringView m_sCurrentTransitionEvent;

  const plStateMachineDescription::TransitionArray* m_pCurrentTransitions = nullptr;

  plBlob m_InstanceData;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plStateMachineInstance);
