#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <GameEngine/StateMachine/StateMachine.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(EditorPluginAssets, StateMachine)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plQtNodeScene::GetPinFactory().RegisterCreator(plGetStaticRTTI<plStateMachinePin>(), [](const plRTTI* pRtti)->plQtPin* { return new plQtStateMachinePin(); });
    plQtNodeScene::GetConnectionFactory().RegisterCreator(plGetStaticRTTI<plStateMachineConnection>(), [](const plRTTI* pRtti)->plQtConnection* { return new plQtStateMachineConnection(); });    
    plQtNodeScene::GetNodeFactory().RegisterCreator(plGetStaticRTTI<plStateMachineNodeBase>(), [](const plRTTI* pRtti)->plQtNode* { return new plQtStateMachineNode(); });
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plQtNodeScene::GetPinFactory().UnregisterCreator(plGetStaticRTTI<plStateMachinePin>());
    plQtNodeScene::GetConnectionFactory().UnregisterCreator(plGetStaticRTTI<plStateMachineConnection>());
    plQtNodeScene::GetNodeFactory().UnregisterCreator(plGetStaticRTTI<plStateMachineNodeBase>());
  }

PLASMA_END_SUBSYSTEM_DECLARATION;

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachinePin, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachinePin::plStateMachinePin(Type type, const plDocumentObject* pObject)
  : plPin(type, type == Type::Input ? "Enter" : "Exit", plColor::Grey, pObject)
{
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineConnection, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Type", m_pType)->AddFlags(plPropertyFlags::PointerOwner)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineNodeBase, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineNode, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new plDefaultValueAttribute(plStringView("State"))), // wrap in plStringView to prevent a memory leak report
    PLASMA_MEMBER_PROPERTY("Type", m_pType)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineNodeAny, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

plStateMachineNodeManager::plStateMachineNodeManager()
{
  m_ObjectEvents.AddEventHandler(plMakeDelegate(&plStateMachineNodeManager::ObjectHandler, this));
}

plStateMachineNodeManager::~plStateMachineNodeManager()
{
  m_ObjectEvents.RemoveEventHandler(plMakeDelegate(&plStateMachineNodeManager::ObjectHandler, this));
}

void plStateMachineNodeManager::SetInitialState(const plDocumentObject* pObject)
{
  if (m_pInitialStateObject == pObject)
    return;

  PLASMA_ASSERT_DEV(IsAnyState(pObject) == false, "'Any State' can't be initial state");

  auto BroadcastEvent = [this](const plDocumentObject* pObject) {
    if (pObject != nullptr)
    {
      plDocumentObjectPropertyEvent e;
      e.m_EventType = plDocumentObjectPropertyEvent::Type::PropertySet;
      e.m_pObject = pObject;

      m_PropertyEvents.Broadcast(e);
    }
  };

  const plDocumentObject* pOldInitialStateObject = m_pInitialStateObject;
  m_pInitialStateObject = pObject;

  // Broadcast after the initial state object has been changed since the qt node will query it from the manager
  BroadcastEvent(pOldInitialStateObject);
  BroadcastEvent(m_pInitialStateObject);
}

bool plStateMachineNodeManager::IsAnyState(const plDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<plStateMachineNodeAny>();
  }
  return false;
}

bool plStateMachineNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  if (pObject != nullptr)
  {
    auto pType = pObject->GetTypeAccessor().GetType();
    return pType->IsDerivedFrom<plStateMachineNodeBase>();
  }
  return false;
}

plStatus plStateMachineNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return plStatus(PLASMA_SUCCESS);
}

void plStateMachineNodeManager::InternalCreatePins(const plDocumentObject* pObject, NodeInternal& node)
{
  if (IsNode(pObject) == false)
    return;

  if (IsAnyState(pObject) == false)
  {
    auto pPin = PLASMA_DEFAULT_NEW(plStateMachinePin, plPin::Type::Input, pObject);
    node.m_Inputs.PushBack(pPin);
  }

  {
    auto pPin = PLASMA_DEFAULT_NEW(plStateMachinePin, plPin::Type::Output, pObject);
    node.m_Outputs.PushBack(pPin);
  }
}

void plStateMachineNodeManager::GetCreateableTypes(plHybridArray<const plRTTI*, 32>& Types) const
{
  Types.PushBack(plGetStaticRTTI<plStateMachineNode>());
  Types.PushBack(plGetStaticRTTI<plStateMachineNodeAny>());
}

const plRTTI* plStateMachineNodeManager::GetConnectionType() const
{
  return plGetStaticRTTI<plStateMachineConnection>();
}

void plStateMachineNodeManager::ObjectHandler(const plDocumentObjectEvent& e)
{
  if (e.m_EventType == plDocumentObjectEvent::Type::AfterObjectCreated && IsNode(e.m_pObject))
  {
    if (m_pInitialStateObject == nullptr && IsAnyState(e.m_pObject) == false)
    {
      SetInitialState(e.m_pObject);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachine_SetInitialStateCommand, 1, plRTTIDefaultAllocator<plStateMachine_SetInitialStateCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("NewInitialStateObject", m_NewInitialStateObject),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachine_SetInitialStateCommand::plStateMachine_SetInitialStateCommand() = default;

plStatus plStateMachine_SetInitialStateCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();
  auto pManager = static_cast<plStateMachineNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pNewInitialStateObject = pManager->GetObject(m_NewInitialStateObject);
    m_pOldInitialStateObject = pManager->GetInitialState();
  }

  pManager->SetInitialState(m_pNewInitialStateObject);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plStateMachine_SetInitialStateCommand::UndoInternal(bool bFireEvents)
{
  plDocument* pDocument = GetDocument();
  auto pManager = static_cast<plStateMachineNodeManager*>(pDocument->GetObjectManager());

  pManager->SetInitialState(m_pOldInitialStateObject);
  return plStatus(PLASMA_SUCCESS);
}
