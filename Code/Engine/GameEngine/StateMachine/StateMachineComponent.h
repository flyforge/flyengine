#pragma once

#include <Core/Messages/EventMessage.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

/// \brief Message that is sent by plStateMachineState_SendMsg once the state is entered.
struct PLASMA_GAMEENGINE_DLL plMsgStateMachineStateChanged : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgStateMachineStateChanged, plEventMessage);

  plHashedString m_sOldStateName;
  plHashedString m_sNewStateName;

private:
  const char* GetOldStateName() const { return m_sOldStateName; }
  void SetOldStateName(const char* szName) { m_sOldStateName.Assign(szName); }

  const char* GetNewStateName() const { return m_sNewStateName; }
  void SetNewStateName(const char* szName) { m_sNewStateName.Assign(szName); }
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine state that sends a plMsgStateMachineStateChanged on state enter or exit to the owner of the
/// state machine instance. Currently only works for plStateMachineComponent.
///
/// Optionally it can also log a message on state enter or exit.
class plStateMachineState_SendMsg : public plStateMachineState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineState_SendMsg, plStateMachineState);

public:
  plStateMachineState_SendMsg(plStringView sName = plStringView());
  ~plStateMachineState_SendMsg();

  virtual void OnEnter(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pFromState) const override;
  virtual void OnExit(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pToState) const override;

  virtual plResult Serialize(plStreamWriter& stream) const override;
  virtual plResult Deserialize(plStreamReader& stream) override;

  plTime m_MessageDelay;

  bool m_bSendMessageOnEnter = true;
  bool m_bSendMessageOnExit = false;
  bool m_bLogOnEnter = false;
  bool m_bLogOnExit = false;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A state machine state that sets the enabled flag on a game object and disables all other objects in the same group.
///
/// This state allows to easily switch the representation of a game object.
/// For instance you may have two objects states: normal and burning
/// You can basically just build two objects, one in the normal state, and one with all the effects needed for the fire.
/// Then you group both objects under a shared parent (e.g. with name 'visuals'), give both of them a name ('normal', 'burning') and disable one of them.
///
/// When the state machine transitions from the normal state to the burning state, you can then use this type of state
/// to say that from the 'visuals' group you want to activate the 'burning' object and deactivate all other objects in the same group.
///
/// Because the state activates one object and deactivates all others, you can have many different visuals and switch between them.
/// You can also only activate an object and keep the rest in the group as they are (e.g. to enable more and more effects).
/// If you only give a group path, but no object name, you can also use it to just disable all objects in a group.
/// If multiple objects in the same group have the same name, they will all get activated simultaneously.
///
/// Make sure that essential other objects (like the physics representation or other scripts) are located on other objects, that don't get deactivated.
class plStateMachineState_SwitchObject : public plStateMachineState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineState_SwitchObject, plStateMachineState);

public:
  plStateMachineState_SwitchObject(plStringView sName = plStringView());
  ~plStateMachineState_SwitchObject();

  virtual void OnEnter(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pFromState) const override;

  virtual plResult Serialize(plStreamWriter& stream) const override;
  virtual plResult Deserialize(plStreamReader& stream) override;

  plString m_sGroupPath;
  plString m_sObjectToEnable;
  bool m_bDeactivateOthers = true;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_GAMEENGINE_DLL plStateMachineComponentManager : public plComponentManager<class plStateMachineComponent, plBlockStorageType::Compact>
{
public:
  plStateMachineComponentManager(plWorld* pWorld);
  ~plStateMachineComponentManager();

  virtual void Initialize() override;

  void Update(const plWorldModule::UpdateContext& context);

private:
  void ResourceEventHandler(const plResourceEvent& e);

  plHashSet<plComponentHandle> m_ComponentsToReload;
};

//////////////////////////////////////////////////////////////////////////

/// \brief A component that holds an plStateMachineInstance using the plStateMachineDescription from the resource assigned to this component.
class PLASMA_GAMEENGINE_DLL plStateMachineComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plStateMachineComponent, plComponent, plStateMachineComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plStateMachineComponent

public:
  plStateMachineComponent();
  plStateMachineComponent(plStateMachineComponent&& other);
  ~plStateMachineComponent();

  plStateMachineComponent& operator=(plStateMachineComponent&& other);

  /// \brief Returns the plStateMachineInstance owned by this component
  plStateMachineInstance* GetStateMachineInstance() { return m_pStateMachineInstance.Borrow(); }
  const plStateMachineInstance* GetStateMachineInstance() const { return m_pStateMachineInstance.Borrow(); }

  void SetResource(const plStateMachineResourceHandle& hResource);
  const plStateMachineResourceHandle& GetResource() const { return m_hResource; }

  void SetResourceFile(const char* szFile); // [ property ]
  const char* GetResourceFile() const;      // [ property ]

  /// \brief Defines which state should be used as initial state after the state machine was instantiated.
  /// If empty the state machine resource defines the initial state.
  void SetInitialState(const char* szName);                       // [ property ]
  const char* GetInitialState() const { return m_sInitialState; } // [ property ]

  /// \brief Sets the current state with the given name.
  bool SetState(plStringView sName); // [ scriptable ]

  /// \brief Returns the name of the currently active state.
  plStringView GetCurrentState() const; // [ scriptable ]

  /// \brief Sends a named event that state transitions can react to.
  void FireTransitionEvent(plStringView sEvent);

  void SetBlackboardName(const char* szName);                         // [ property ]
  const char* GetBlackboardName() const { return m_sBlackboardName; } // [ property ]

private:
  friend class plStateMachineState_SendMsg;
  void SendStateChangedMsg(plMsgStateMachineStateChanged& msg, plTime delay);
  void InstantiateStateMachine();
  void Update();

  plStateMachineResourceHandle m_hResource;
  plHashedString m_sInitialState;
  plHashedString m_sBlackboardName;

  plUniquePtr<plStateMachineInstance> m_pStateMachineInstance;

  plEventMessageSender<plMsgStateMachineStateChanged> m_StateChangedSender; // [ event ]
};
