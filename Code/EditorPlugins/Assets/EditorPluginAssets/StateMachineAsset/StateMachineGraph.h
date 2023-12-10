#pragma once

#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

class plStateMachineState;
class plStateMachineTransition;

class plStateMachinePin : public plPin
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachinePin, plPin);

public:
  plStateMachinePin(Type type, const plDocumentObject* pObject);
};

/// \brief A connection that represents a state machine transition. Since we can't chose different connection
/// types in the Editor we allow the user to switch the type in the properties.
class plStateMachineConnection : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineConnection, plReflectedClass);

public:
  plStateMachineTransition* m_pType = nullptr;
};

/// \brief Base class for nodes in the state machine graph
class plStateMachineNodeBase : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineNodeBase, plReflectedClass);
};

/// \brief A node that represents a state machine state. We don't use plStateMachineState directly to allow
/// the user to switch the type in the properties similar to what we do with transitions.
class plStateMachineNode : public plStateMachineNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineNode, plStateMachineNodeBase);

public:
  plString m_sName;
  plStateMachineState* m_pType = nullptr;
};

/// \brief A node that represents "any" state machine state. This can be used if a transition with the same conditions
/// is possible from any other state in the state machine. Instead of creating many connections with the same properties
/// an "any" state can be used to make the graph much easier to read and to maintain.
///
/// Note that there is no "any" state at runtime but rather only the transition is stored.
class plStateMachineNodeAny : public plStateMachineNodeBase
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineNodeAny, plStateMachineNodeBase);
};

class plStateMachineNodeManager : public plDocumentNodeManager
{
public:
  plStateMachineNodeManager();
  ~plStateMachineNodeManager();

  bool IsInitialState(const plDocumentObject* pObject) const { return pObject == m_pInitialStateObject; }
  void SetInitialState(const plDocumentObject* pObject);
  const plDocumentObject* GetInitialState() const { return m_pInitialStateObject; }

  bool IsAnyState(const plDocumentObject* pObject) const;

private:
  virtual bool InternalIsNode(const plDocumentObject* pObject) const override;
  virtual plStatus InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_Result) const override;

  virtual void InternalCreatePins(const plDocumentObject* pObject, NodeInternal& node) override;

  virtual void GetCreateableTypes(plHybridArray<const plRTTI*, 32>& Types) const override;
  virtual const plRTTI* GetConnectionType() const override;

  void ObjectHandler(const plDocumentObjectEvent& e);

private:
  const plDocumentObject* m_pInitialStateObject = nullptr;
};

class plStateMachine_SetInitialStateCommand : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachine_SetInitialStateCommand, plCommand);

public:
  plStateMachine_SetInitialStateCommand();

public: // Properties
  plUuid m_NewInitialStateObject;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  const plDocumentObject* m_pOldInitialStateObject = nullptr;
  const plDocumentObject* m_pNewInitialStateObject = nullptr;
};
