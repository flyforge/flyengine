#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/NodeObject/NodeCommandAccessor.h>

plNodeCommandAccessor::plNodeCommandAccessor(plCommandHistory* pHistory)
  : plObjectCommandAccessor(pHistory)
{
}

plNodeCommandAccessor::~plNodeCommandAccessor() = default;

plStatus plNodeCommandAccessor::SetValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  if (m_pHistory->InTemporaryTransaction() == false)
  {
    auto pNodeObject = pObject;
    auto pDynamicPinProperty = pProp;

    if (IsNode(pObject) == false)
    {
      auto pParent = pObject->GetParent();
      if (pParent != nullptr && IsNode(pParent))
      {
        pNodeObject = pParent;
        pDynamicPinProperty = pParent->GetType()->FindPropertyByName(pObject->GetParentProperty());
      }
    }

    if (IsDynamicPinProperty(pNodeObject, pDynamicPinProperty))
    {
      plHybridArray<ConnectionInfo, 16> oldConnections;
      PL_SUCCEED_OR_RETURN(DisconnectAllPins(pNodeObject, oldConnections));

      // TODO: remap oldConnections

      PL_SUCCEED_OR_RETURN(plObjectCommandAccessor::SetValue(pObject, pProp, newValue, index));

      return TryReconnectAllPins(pNodeObject, oldConnections);
    }
  }

  return plObjectCommandAccessor::SetValue(pObject, pProp, newValue, index);
}

plStatus plNodeCommandAccessor::InsertValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    plHybridArray<ConnectionInfo, 16> oldConnections;
    PL_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    PL_SUCCEED_OR_RETURN(plObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return plObjectCommandAccessor::InsertValue(pObject, pProp, newValue, index);
  }
}

plStatus plNodeCommandAccessor::RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index /*= plVariant()*/)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    plHybridArray<ConnectionInfo, 16> oldConnections;
    PL_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    PL_SUCCEED_OR_RETURN(plObjectCommandAccessor::RemoveValue(pObject, pProp, index));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return plObjectCommandAccessor::RemoveValue(pObject, pProp, index);
  }
}

plStatus plNodeCommandAccessor::MoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex)
{
  if (IsDynamicPinProperty(pObject, pProp))
  {
    plHybridArray<ConnectionInfo, 16> oldConnections;
    PL_SUCCEED_OR_RETURN(DisconnectAllPins(pObject, oldConnections));

    // TODO: remap oldConnections

    PL_SUCCEED_OR_RETURN(plObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex));

    return TryReconnectAllPins(pObject, oldConnections);
  }
  else
  {
    return plObjectCommandAccessor::MoveValue(pObject, pProp, oldIndex, newIndex);
  }
}

plStatus plNodeCommandAccessor::AddObject(const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid)
{
  if (IsDynamicPinProperty(pParent, pParentProp))
  {
    plHybridArray<ConnectionInfo, 16> oldConnections;
    PL_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

    // TODO: remap oldConnections

    PL_SUCCEED_OR_RETURN(plObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid));

    return TryReconnectAllPins(pParent, oldConnections);
  }
  else
  {
    return plObjectCommandAccessor::AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
  }
}

plStatus plNodeCommandAccessor::RemoveObject(const plDocumentObject* pObject)
{
  if (const plDocumentObject* pParent = pObject->GetParent())
  {
    const plAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(pObject->GetParentProperty());
    if (IsDynamicPinProperty(pParent, pProp))
    {
      plHybridArray<ConnectionInfo, 16> oldConnections;
      PL_SUCCEED_OR_RETURN(DisconnectAllPins(pParent, oldConnections));

      // TODO: remap oldConnections

      PL_SUCCEED_OR_RETURN(plObjectCommandAccessor::RemoveObject(pObject));

      return TryReconnectAllPins(pParent, oldConnections);
    }
  }

  return plObjectCommandAccessor::RemoveObject(pObject);
}


bool plNodeCommandAccessor::IsNode(const plDocumentObject* pObject) const
{
  auto pManager = static_cast<const plDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsNode(pObject);
}

bool plNodeCommandAccessor::IsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const
{
  auto pManager = static_cast<const plDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  return pManager->IsDynamicPinProperty(pObject, pProp);
}

plStatus plNodeCommandAccessor::DisconnectAllPins(const plDocumentObject* pObject, plDynamicArray<ConnectionInfo>& out_oldConnections)
{
  auto pManager = static_cast<const plDocumentNodeManager*>(pObject->GetDocumentObjectManager());

  auto Disconnect = [&](plArrayPtr<const plConnection* const> connections) -> plStatus {
    for (const plConnection* pConnection : connections)
    {
      auto& connectionInfo = out_oldConnections.ExpandAndGetRef();
      connectionInfo.m_pSource = pConnection->GetSourcePin().GetParent();
      connectionInfo.m_pTarget = pConnection->GetTargetPin().GetParent();
      connectionInfo.m_sSourcePin = pConnection->GetSourcePin().GetName();
      connectionInfo.m_sTargetPin = pConnection->GetTargetPin().GetName();

      PL_SUCCEED_OR_RETURN(plNodeCommands::DisconnectAndRemoveCommand(m_pHistory, pConnection->GetParent()->GetGuid()));
    }

    return plStatus(PL_SUCCESS);
  };

  auto inputs = pManager->GetInputPins(pObject);
  for (auto& pInputPin : inputs)
  {
    PL_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pInputPin)));
  }

  auto outputs = pManager->GetOutputPins(pObject);
  for (auto& pOutputPin : outputs)
  {
    PL_SUCCEED_OR_RETURN(Disconnect(pManager->GetConnections(*pOutputPin)));
  }

  return plStatus(PL_SUCCESS);
}

plStatus plNodeCommandAccessor::TryReconnectAllPins(const plDocumentObject* pObject, const plDynamicArray<ConnectionInfo>& oldConnections)
{
  auto pManager = static_cast<const plDocumentNodeManager*>(pObject->GetDocumentObjectManager());
  const plRTTI* pConnectionType = pManager->GetConnectionType();

  for (auto& connectionInfo : oldConnections)
  {
    const plPin* pSourcePin = pManager->GetOutputPinByName(connectionInfo.m_pSource, connectionInfo.m_sSourcePin);
    const plPin* pTargetPin = pManager->GetInputPinByName(connectionInfo.m_pTarget, connectionInfo.m_sTargetPin);

    // This connection can't be restored because a pin doesn't exist anymore, which is ok in this case.
    if (pSourcePin == nullptr || pTargetPin == nullptr)
      continue;

    // This connection is not valid anymore after pins have changed.
    plDocumentNodeManager::CanConnectResult res;
    if (pManager->CanConnect(pConnectionType, *pSourcePin, *pTargetPin, res).Failed())
      continue;

    PL_SUCCEED_OR_RETURN(plNodeCommands::AddAndConnectCommand(m_pHistory, pConnectionType, *pSourcePin, *pTargetPin));
  }

  return plStatus(PL_SUCCESS);
}
