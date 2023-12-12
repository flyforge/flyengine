#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRemoveNodeCommand, 1, plRTTIDefaultAllocator<plRemoveNodeCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMoveNodeCommand, 1, plRTTIDefaultAllocator<plMoveNodeCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ObjectGuid", m_Object),
    PLASMA_MEMBER_PROPERTY("NewPos", m_NewPos),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plConnectNodePinsCommand, 1, plRTTIDefaultAllocator<plConnectNodePinsCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
    PLASMA_MEMBER_PROPERTY("SourceGuid", m_ObjectSource),
    PLASMA_MEMBER_PROPERTY("TargetGuid", m_ObjectTarget),
    PLASMA_MEMBER_PROPERTY("SourcePin", m_sSourcePin),
    PLASMA_MEMBER_PROPERTY("TargetPin", m_sTargetPin),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDisconnectNodePinsCommand, 1, plRTTIDefaultAllocator<plDisconnectNodePinsCommand>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ConnectionGuid", m_ConnectionObject),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plRemoveNodeCommand
////////////////////////////////////////////////////////////////////////

plRemoveNodeCommand::plRemoveNodeCommand() = default;

plStatus plRemoveNodeCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(pDocument->GetObjectManager());

  auto RemoveConnections = [&](const plPin& pin) {
    while (true)
    {
      auto connections = pManager->GetConnections(pin);

      if (connections.IsEmpty())
        break;

      plDisconnectNodePinsCommand cmd;
      cmd.m_ConnectionObject = connections[0]->GetParent()->GetGuid();
      plStatus res = AddSubCommand(cmd);
      if (res.m_Result.Succeeded())
      {
        plRemoveObjectCommand remove;
        remove.m_Object = cmd.m_ConnectionObject;
        res = AddSubCommand(remove);
      }

      PLASMA_SUCCEED_OR_RETURN(res);
    }
    return plStatus(PLASMA_SUCCESS);
  };

  if (!bRedo)
  {
    m_pObject = pManager->GetObject(m_Object);
    if (m_pObject == nullptr)
      return plStatus("Remove Node: The given object does not exist!");

    auto inputs = pManager->GetInputPins(m_pObject);
    for (auto& pPinTarget : inputs)
    {
      PLASMA_SUCCEED_OR_RETURN(RemoveConnections(*pPinTarget));
    }

    auto outputs = pManager->GetOutputPins(m_pObject);
    for (auto& pPinSource : outputs)
    {
      PLASMA_SUCCEED_OR_RETURN(RemoveConnections(*pPinSource));
    }

    plRemoveObjectCommand cmd;
    cmd.m_Object = m_Object;
    auto res = AddSubCommand(cmd);
    if (res.m_Result.Failed())
    {
      return res;
    }
  }
  return plStatus(PLASMA_SUCCESS);
}

plStatus plRemoveNodeCommand::UndoInternal(bool bFireEvents)
{
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  return plStatus(PLASMA_SUCCESS);
}

void plRemoveNodeCommand::CleanupInternal(CommandState state) {}


////////////////////////////////////////////////////////////////////////
// plMoveObjectCommand
////////////////////////////////////////////////////////////////////////

plMoveNodeCommand::plMoveNodeCommand() = default;

plStatus plMoveNodeCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return plStatus("Move Node: The given object does not exist!");

    m_vOldPos = pManager->GetNodePos(m_pObject);
    PLASMA_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_NewPos));
  }

  pManager->MoveNode(m_pObject, m_NewPos);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plMoveNodeCommand::UndoInternal(bool bFireEvents)
{
  plDocument* pDocument = GetDocument();
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(pDocument->GetObjectManager());
  PLASMA_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  PLASMA_SUCCEED_OR_RETURN(pManager->CanMoveNode(m_pObject, m_vOldPos));

  pManager->MoveNode(m_pObject, m_vOldPos);

  return plStatus(PLASMA_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// plConnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

plConnectNodePinsCommand::plConnectNodePinsCommand() = default;

plStatus plConnectNodePinsCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return plStatus("Connect Node Pins: The given connection object is not valid connection!");

    m_pObjectSource = pManager->GetObject(m_ObjectSource);
    if (m_pObjectSource == nullptr)
      return plStatus("Connect Node Pins: The given node does not exist!");
    m_pObjectTarget = pManager->GetObject(m_ObjectTarget);
    if (m_pObjectTarget == nullptr)
      return plStatus("Connect Node Pins: The given node does not exist!");
  }

  const plPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return plStatus("Connect Node Pins: The given pin does not exist!");

  const plPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return plStatus("Connect Node Pins: The given pin does not exist!");

  plDocumentNodeManager::CanConnectResult res;
  PLASMA_SUCCEED_OR_RETURN(pManager->CanConnect(m_pConnectionObject->GetType(), *pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plConnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  plDocument* pDocument = GetDocument();
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(pDocument->GetObjectManager());

  PLASMA_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);
  return plStatus(PLASMA_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// plDisconnectNodePinsCommand
////////////////////////////////////////////////////////////////////////

plDisconnectNodePinsCommand::plDisconnectNodePinsCommand() = default;

plStatus plDisconnectNodePinsCommand::DoInternal(bool bRedo)
{
  plDocument* pDocument = GetDocument();
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(pDocument->GetObjectManager());

  if (!bRedo)
  {
    m_pConnectionObject = pManager->GetObject(m_ConnectionObject);
    if (!pManager->IsConnection(m_pConnectionObject))
      return plStatus("Disconnect Node Pins: The given connection object is not valid connection!");

    PLASMA_SUCCEED_OR_RETURN(pManager->CanRemove(m_pConnectionObject));

    const plConnection& connection = pManager->GetConnection(m_pConnectionObject);
    const plPin& pinSource = connection.GetSourcePin();
    const plPin& pinTarget = connection.GetTargetPin();

    m_pObjectSource = pinSource.GetParent();
    m_pObjectTarget = pinTarget.GetParent();
    m_sSourcePin = pinSource.GetName();
    m_sTargetPin = pinTarget.GetName();
  }

  PLASMA_SUCCEED_OR_RETURN(pManager->CanDisconnect(m_pConnectionObject));

  pManager->Disconnect(m_pConnectionObject);

  return plStatus(PLASMA_SUCCESS);
}

plStatus plDisconnectNodePinsCommand::UndoInternal(bool bFireEvents)
{
  plDocument* pDocument = GetDocument();
  plDocumentNodeManager* pManager = static_cast<plDocumentNodeManager*>(pDocument->GetObjectManager());

  const plPin* pOutput = pManager->GetOutputPinByName(m_pObjectSource, m_sSourcePin);
  if (pOutput == nullptr)
    return plStatus("Connect Node: The given pin does not exist!");

  const plPin* pInput = pManager->GetInputPinByName(m_pObjectTarget, m_sTargetPin);
  if (pInput == nullptr)
    return plStatus("Connect Node: The given pin does not exist!");

  plDocumentNodeManager::CanConnectResult res;
  PLASMA_SUCCEED_OR_RETURN(pManager->CanConnect(m_pConnectionObject->GetType(), *pOutput, *pInput, res));

  pManager->Connect(m_pConnectionObject, *pOutput, *pInput);
  return plStatus(PLASMA_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// plNodeCommands
////////////////////////////////////////////////////////////////////////

// static
plStatus plNodeCommands::AddAndConnectCommand(plCommandHistory* history, const plRTTI* pConnectionType, const plPin& sourcePin, const plPin& targetPin)
{
  plAddObjectCommand cmd;
  cmd.m_pType = pConnectionType;
  cmd.m_NewObjectGuid.CreateNewUuid();
  cmd.m_Index = -1;

  plStatus res = history->AddCommand(cmd);
  if (res.m_Result.Succeeded())
  {
    plConnectNodePinsCommand connect;
    connect.m_ConnectionObject = cmd.m_NewObjectGuid;
    connect.m_ObjectSource = sourcePin.GetParent()->GetGuid();
    connect.m_ObjectTarget = targetPin.GetParent()->GetGuid();
    connect.m_sSourcePin = sourcePin.GetName();
    connect.m_sTargetPin = targetPin.GetName();

    res = history->AddCommand(connect);
  }

  return res;
}

// static
plStatus plNodeCommands::DisconnectAndRemoveCommand(plCommandHistory* history, const plUuid& connectionObject)
{
  plDisconnectNodePinsCommand cmd;
  cmd.m_ConnectionObject = connectionObject;

  plStatus res = history->AddCommand(cmd);
  if (res.m_Result.Succeeded())
  {
    plRemoveObjectCommand remove;
    remove.m_Object = cmd.m_ConnectionObject;

    res = history->AddCommand(remove);
  }

  return res;
}
