#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/NodeObject/DocumentNodeManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plPin, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plDocumentNodeManager Internal
////////////////////////////////////////////////////////////////////////

struct DocumentNodeManager_NodeMetaData
{
  plVec2 m_Pos = plVec2::MakeZero();
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, DocumentNodeManager_NodeMetaData);

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_NodeMetaData, plNoBase, 1, plRTTIDefaultAllocator<DocumentNodeManager_NodeMetaData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Node::Pos", m_Pos),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

struct DocumentNodeManager_ConnectionMetaData
{
  plUuid m_Source;
  plUuid m_Target;
  plString m_SourcePin;
  plString m_TargetPin;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, DocumentNodeManager_ConnectionMetaData);

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(DocumentNodeManager_ConnectionMetaData, plNoBase, 1, plRTTIDefaultAllocator<DocumentNodeManager_ConnectionMetaData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Connection::Source", m_Source),
    PLASMA_MEMBER_PROPERTY("Connection::Target", m_Target),
    PLASMA_MEMBER_PROPERTY("Connection::SourcePin", m_SourcePin),
    PLASMA_MEMBER_PROPERTY("Connection::TargetPin", m_TargetPin),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

class DocumentNodeManager_DefaultConnection : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(DocumentNodeManager_DefaultConnection, plReflectedClass);
};

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(DocumentNodeManager_DefaultConnection, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plDocumentNodeManager
////////////////////////////////////////////////////////////////////////

plDocumentNodeManager::plDocumentNodeManager()
{
  m_ObjectEvents.AddEventHandler(plMakeDelegate(&plDocumentNodeManager::ObjectHandler, this));
  m_StructureEvents.AddEventHandler(plMakeDelegate(&plDocumentNodeManager::StructureEventHandler, this));
  m_PropertyEvents.AddEventHandler(plMakeDelegate(&plDocumentNodeManager::PropertyEventsHandler, this));
}

plDocumentNodeManager::~plDocumentNodeManager()
{
  m_ObjectEvents.RemoveEventHandler(plMakeDelegate(&plDocumentNodeManager::ObjectHandler, this));
  m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plDocumentNodeManager::StructureEventHandler, this));
  m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plDocumentNodeManager::PropertyEventsHandler, this));
}

const plRTTI* plDocumentNodeManager::GetConnectionType() const
{
  return plGetStaticRTTI<DocumentNodeManager_DefaultConnection>();
}

plVec2 plDocumentNodeManager::GetNodePos(const plDocumentObject* pObject) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEV(it.IsValid(), "Can't get pos of objects that aren't nodes!");
  return it.Value().m_vPos;
}

const plConnection& plDocumentNodeManager::GetConnection(const plDocumentObject* pObject) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEV(it.IsValid(), "Can't get connection for objects that aren't connections!");
  return *it.Value();
}

const plPin* plDocumentNodeManager::GetInputPinByName(const plDocumentObject* pObject, plStringView sName) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Inputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

const plPin* plDocumentNodeManager::GetOutputPinByName(const plDocumentObject* pObject, plStringView sName) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  for (auto& pPin : it.Value().m_Outputs)
  {
    if (pPin->GetName() == sName)
      return pPin.Borrow();
  }
  return nullptr;
}

plArrayPtr<const plUniquePtr<const plPin>> plDocumentNodeManager::GetInputPins(const plDocumentObject* pObject) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return plMakeArrayPtr((plUniquePtr<const plPin>*)it.Value().m_Inputs.GetData(), it.Value().m_Inputs.GetCount());
}

plArrayPtr<const plUniquePtr<const plPin>> plDocumentNodeManager::GetOutputPins(const plDocumentObject* pObject) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEV(it.IsValid(), "Can't get input pins of objects that aren't nodes!");
  return plMakeArrayPtr((plUniquePtr<const plPin>*)it.Value().m_Outputs.GetData(), it.Value().m_Outputs.GetCount());
}

bool plDocumentNodeManager::IsNode(const plDocumentObject* pObject) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsNode(pObject);
}

bool plDocumentNodeManager::IsConnection(const plDocumentObject* pObject) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (pObject == nullptr)
    return false;
  if (pObject == GetRootObject())
    return false;

  return InternalIsConnection(pObject);
}

bool plDocumentNodeManager::IsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const
{
  if (IsNode(pObject) == false)
    return false;

  if (pProp == nullptr)
    return false;

  return InternalIsDynamicPinProperty(pObject, pProp);
}

plArrayPtr<const plConnection* const> plDocumentNodeManager::GetConnections(const plPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  if (it.IsValid())
  {
    return it.Value();
  }

  return plArrayPtr<const plConnection* const>();
}

bool plDocumentNodeManager::HasConnections(const plPin& pin) const
{
  auto it = m_Connections.Find(&pin);
  return it.IsValid() && it.Value().IsEmpty() == false;
}

bool plDocumentNodeManager::IsConnected(const plPin& source, const plPin& target) const
{
  auto it = m_Connections.Find(&source);
  if (it.IsValid())
  {
    for (auto pConnection : it.Value())
    {
      if (&pConnection->GetTargetPin() == &target)
        return true;
    }
  }

  return false;
}

plStatus plDocumentNodeManager::CanConnect(const plRTTI* pObjectType, const plPin& source, const plPin& target, CanConnectResult& out_result) const
{
  out_result = CanConnectResult::ConnectNever;

  if (pObjectType == nullptr || pObjectType->IsDerivedFrom(GetConnectionType()) == false)
    return plStatus("Invalid connection object type");

  if (source.m_Type != plPin::Type::Output)
    return plStatus("Source pin is not an output pin.");
  if (target.m_Type != plPin::Type::Input)
    return plStatus("Target pin is not an input pin.");

  if (source.m_pParent == target.m_pParent)
    return plStatus("Nodes cannot be connect with themselves.");

  if (IsConnected(source, target))
    return plStatus("Pins already connected.");

  return InternalCanConnect(source, target, out_result);
}

plStatus plDocumentNodeManager::CanDisconnect(const plConnection* pConnection) const
{
  if (pConnection == nullptr)
    return plStatus("Invalid connection");

  return InternalCanDisconnect(pConnection->GetSourcePin(), pConnection->GetTargetPin());
}

plStatus plDocumentNodeManager::CanDisconnect(const plDocumentObject* pObject) const
{
  if (!IsConnection(pObject))
    return plStatus("Invalid connection object");

  const plConnection& connection = GetConnection(pObject);
  return InternalCanDisconnect(connection.GetSourcePin(), connection.GetTargetPin());
}

plStatus plDocumentNodeManager::CanMoveNode(const plDocumentObject* pObject, const plVec2& vPos) const
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "Invalid input!");
  if (!IsNode(pObject))
    return plStatus("The given object is not a node!");

  return InternalCanMoveNode(pObject, vPos);
}

void plDocumentNodeManager::Connect(const plDocumentObject* pObject, const plPin& source, const plPin& target)
{
  plDocumentNodeManager::CanConnectResult res = CanConnectResult::ConnectNever;
  PLASMA_IGNORE_UNUSED(res);
  PLASMA_ASSERT_DEBUG(CanConnect(pObject->GetType(), source, target, res).m_Result.Succeeded(), "Connect: Sanity check failed!");

  auto pConnection = PLASMA_DEFAULT_NEW(plConnection, source, target, pObject);
  m_ObjectToConnection.Insert(pObject->GetGuid(), pConnection);

  m_Connections[&source].PushBack(pConnection);
  m_Connections[&target].PushBack(pConnection);

  {
    plDocumentNodeManagerEvent e(plDocumentNodeManagerEvent::Type::AfterPinsConnected, pObject);
    m_NodeEvents.Broadcast(e);
  }
}

void plDocumentNodeManager::Disconnect(const plDocumentObject* pObject)
{
  auto it = m_ObjectToConnection.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");
  PLASMA_ASSERT_DEBUG(CanDisconnect(pObject).m_Result.Succeeded(), "Disconnect: Sanity check failed!");

  {
    plDocumentNodeManagerEvent e(plDocumentNodeManagerEvent::Type::BeforePinsDisonnected, pObject);
    m_NodeEvents.Broadcast(e);
  }

  auto& pConnection = it.Value();
  const plPin& source = pConnection->GetSourcePin();
  const plPin& target = pConnection->GetTargetPin();
  m_Connections[&source].RemoveAndCopy(pConnection.Borrow());
  m_Connections[&target].RemoveAndCopy(pConnection.Borrow());

  m_ObjectToConnection.Remove(it);
}

void plDocumentNodeManager::MoveNode(const plDocumentObject* pObject, const plVec2& vPos)
{
  PLASMA_ASSERT_DEBUG(CanMoveNode(pObject, vPos).m_Result.Succeeded(), "MoveNode: Sanity check failed!");

  auto it = m_ObjectToNode.Find(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(it.IsValid(), "Moveable node does not exist, CanMoveNode impl invalid!");
  it.Value().m_vPos = vPos;

  plDocumentNodeManagerEvent e(plDocumentNodeManagerEvent::Type::NodeMoved, pObject);
  m_NodeEvents.Broadcast(e);
}

void plDocumentNodeManager::AttachMetaDataBeforeSaving(plAbstractObjectGraph& ref_graph) const
{
  auto pNodeMetaDataType = plGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = plGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  plRttiConverterContext context;
  plRttiConverterWriter rttiConverter(&ref_graph, &context, true, true);

  for (auto it = ref_graph.GetAllNodes().GetIterator(); it.IsValid(); ++it)
  {
    auto* pAbstractObject = it.Value();
    const plUuid& guid = pAbstractObject->GetGuid();

    {
      auto it2 = m_ObjectToNode.Find(guid);
      if (it2.IsValid())
      {
        const NodeInternal& node = it2.Value();

        DocumentNodeManager_NodeMetaData nodeMetaData;
        nodeMetaData.m_Pos = node.m_vPos;
        rttiConverter.AddProperties(pAbstractObject, pNodeMetaDataType, &nodeMetaData);
      }
    }

    {
      auto it2 = m_ObjectToConnection.Find(guid);
      if (it2.IsValid())
      {
        const plConnection& connection = *it2.Value();
        const plPin& sourcePin = connection.GetSourcePin();
        const plPin& targetPin = connection.GetTargetPin();

        DocumentNodeManager_ConnectionMetaData connectionMetaData;
        connectionMetaData.m_Source = sourcePin.GetParent()->GetGuid();
        connectionMetaData.m_Target = targetPin.GetParent()->GetGuid();
        connectionMetaData.m_SourcePin = sourcePin.GetName();
        connectionMetaData.m_TargetPin = targetPin.GetName();
        rttiConverter.AddProperties(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);
      }
    }
  }
}

void plDocumentNodeManager::RestoreMetaDataAfterLoading(const plAbstractObjectGraph& graph, bool bUndoable)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();

  auto pNodeMetaDataType = plGetStaticRTTI<DocumentNodeManager_NodeMetaData>();
  auto pConnectionMetaDataType = plGetStaticRTTI<DocumentNodeManager_ConnectionMetaData>();

  plRttiConverterContext context;
  plRttiConverterReader rttiConverter(&graph, &context);

  for (auto it : graph.GetAllNodes())
  {
    auto pAbstractObject = it.Value();
    plDocumentObject* pObject = GetObject(pAbstractObject->GetGuid());
    if (pObject == nullptr)
      continue;

    if (IsNode(pObject))
    {
      DocumentNodeManager_NodeMetaData nodeMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pNodeMetaDataType, &nodeMetaData);

      if (CanMoveNode(pObject, nodeMetaData.m_Pos).m_Result.Succeeded())
      {
        if (bUndoable)
        {
          plMoveNodeCommand move;
          move.m_Object = pObject->GetGuid();
          move.m_NewPos = nodeMetaData.m_Pos;
          history->AddCommand(move).LogFailure();
        }
        else
        {
          MoveNode(pObject, nodeMetaData.m_Pos);
        }
      }

      // Backwards compatibility to old file format
      if (auto pOldConnections = pAbstractObject->FindProperty("Node::Connections"))
      {
        PLASMA_ASSERT_DEV(bUndoable == false, "Undo not supported for old file format");
        RestoreOldMetaDataAfterLoading(graph, *pOldConnections, pObject);
      }
    }
    else if (IsConnection(pObject))
    {
      DocumentNodeManager_ConnectionMetaData connectionMetaData;
      rttiConverter.ApplyPropertiesToObject(pAbstractObject, pConnectionMetaDataType, &connectionMetaData);

      plDocumentObject* pSource = GetObject(connectionMetaData.m_Source);
      plDocumentObject* pTarget = GetObject(connectionMetaData.m_Target);
      if (pSource == nullptr || pTarget == nullptr)
      {
        RemoveObject(pObject);
        DestroyObject(pObject);
        continue;
      }

      const plPin* pSourcePin = GetOutputPinByName(pSource, connectionMetaData.m_SourcePin);
      if (pSourcePin == nullptr)
      {
        plLog::Error("Unknown output pin '{}' on '{}'. The connection has been removed.", connectionMetaData.m_SourcePin, pSource->GetType()->GetTypeName());
        RemoveObject(pObject);
        DestroyObject(pObject);
        continue;
      }

      const plPin* pTargetPin = GetInputPinByName(pTarget, connectionMetaData.m_TargetPin);
      if (pTargetPin == nullptr)
      {
        plLog::Error("Unknown input pin '{}' on '{}'. The connection has been removed.", connectionMetaData.m_TargetPin, pTarget->GetType()->GetTypeName());
        RemoveObject(pObject);
        DestroyObject(pObject);
        continue;
      }

      plDocumentNodeManager::CanConnectResult res;
      if (CanConnect(pObject->GetType(), *pSourcePin, *pTargetPin, res).m_Result.Failed())
      {
        RemoveObject(pObject);
        DestroyObject(pObject);
        continue;
      }

      if (bUndoable)
      {
        plConnectNodePinsCommand cmd;
        cmd.m_ConnectionObject = pObject->GetGuid();
        cmd.m_ObjectSource = connectionMetaData.m_Source;
        cmd.m_ObjectTarget = connectionMetaData.m_Target;
        cmd.m_sSourcePin = connectionMetaData.m_SourcePin;
        cmd.m_sTargetPin = connectionMetaData.m_TargetPin;
        history->AddCommand(cmd).LogFailure();
      }
      else
      {
        Connect(pObject, *pSourcePin, *pTargetPin);
      }
    }
  }
}

void plDocumentNodeManager::GetMetaDataHash(const plDocumentObject* pObject, plUInt64& inout_uiHash) const
{
  if (IsNode(pObject))
  {
    // The node position is not hashed here since the hash is only used for asset transform
    // and for that the node position is irrelevant.
  }
  else if (IsConnection(pObject))
  {
    const plConnection& connection = GetConnection(pObject);
    const plPin& sourcePin = connection.GetSourcePin();
    const plPin& targetPin = connection.GetTargetPin();

    inout_uiHash = plHashingUtils::xxHash64(&sourcePin.GetParent()->GetGuid(), sizeof(plUuid), inout_uiHash);
    inout_uiHash = plHashingUtils::xxHash64(&targetPin.GetParent()->GetGuid(), sizeof(plUuid), inout_uiHash);
    inout_uiHash = plHashingUtils::xxHash64String(sourcePin.GetName(), inout_uiHash);
    inout_uiHash = plHashingUtils::xxHash64String(targetPin.GetName(), inout_uiHash);
  }
}

bool plDocumentNodeManager::CopySelectedObjects(plAbstractObjectGraph& out_objectGraph) const
{
  const auto& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (selection.IsEmpty())
    return false;

  plDocumentObjectConverterWriter writer(&out_objectGraph, this);

  plHashSet<const plDocumentObject*> copiedNodes;
  for (const plDocumentObject* pObject : selection)
  {
    // Only add nodes here, connections are then collected below to ensure
    // that we always include only valid connections within the copied subgraph no matter if they are selected or not.
    if (IsNode(pObject))
    {
      // objects are required to be named root but this is not enforced or obvious by the interface.
      writer.AddObjectToGraph(pObject, "root");
      copiedNodes.Insert(pObject);
    }
  }

  plHashSet<const plDocumentObject*> copiedConnections;
  for (const plDocumentObject* pNodeObject : selection)
  {
    if (IsNode(pNodeObject) == false)
      continue;

    auto outputs = GetOutputPins(pNodeObject);
    for (auto& pSourcePin : outputs)
    {
      auto connections = GetConnections(*pSourcePin);
      for (const plConnection* pConnection : connections)
      {
        const plDocumentObject* pConnectionObject = pConnection->GetParent();

        PLASMA_ASSERT_DEV(pSourcePin == &pConnection->GetSourcePin(), "");
        if (copiedConnections.Contains(pConnectionObject) == false && copiedNodes.Contains(pConnection->GetTargetPin().GetParent()))
        {
          writer.AddObjectToGraph(pConnectionObject, "root");
          copiedConnections.Insert(pConnectionObject);
        }
      }
    }
  }

  AttachMetaDataBeforeSaving(out_objectGraph);

  return true;
}

bool plDocumentNodeManager::PasteObjects(const plArrayPtr<plDocument::PasteInfo>& info, const plAbstractObjectGraph& objectGraph, const plVec2& vPickedPosition, bool bAllowPickedPosition)
{
  bool bAddedAll = true;
  plDeque<const plDocumentObject*> AddedObjects;

  for (const auto& pi : info)
  {
    // only add nodes that are allowed to be added
    if (CanAdd(pi.m_pObject->GetTypeAccessor().GetType(), nullptr, "Children", pi.m_Index).m_Result.Succeeded())
    {
      AddedObjects.PushBack(pi.m_pObject);
      AddObject(pi.m_pObject, nullptr, "Children", pi.m_Index);
    }
    else
    {
      bAddedAll = false;
    }
  }

  RestoreMetaDataAfterLoading(objectGraph, true);

  if (!AddedObjects.IsEmpty() && bAllowPickedPosition)
  {
    plCommandHistory* history = GetDocument()->GetCommandHistory();

    plVec2 vAvgPos(0);
    plUInt32 nodeCount = 0;
    for (const plDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject))
      {
        vAvgPos += GetNodePos(pObject);
        ++nodeCount;
      }
    }

    vAvgPos /= (float)nodeCount;
    const plVec2 vMoveNode = -vAvgPos + vPickedPosition;

    for (const plDocumentObject* pObject : AddedObjects)
    {
      if (IsNode(pObject))
      {
        plMoveNodeCommand move;
        move.m_Object = pObject->GetGuid();
        move.m_NewPos = GetNodePos(pObject) + vMoveNode;
        history->AddCommand(move).LogFailure();
      }
    }

    if (!bAddedAll)
    {
      plLog::Info("[EditorStatus]Not all nodes were allowed to be added to the document");
    }
  }

  GetDocument()->GetSelectionManager()->SetSelection(AddedObjects);
  return true;
}

bool plDocumentNodeManager::CanReachNode(const plDocumentObject* pSource, const plDocumentObject* pTarget, plSet<const plDocumentObject*>& Visited) const
{
  if (pSource == pTarget)
    return true;

  if (Visited.Contains(pSource))
    return false;

  Visited.Insert(pSource);

  auto outputs = GetOutputPins(pSource);
  for (auto& pSourcePin : outputs)
  {
    auto connections = GetConnections(*pSourcePin);
    for (const plConnection* pConnection : connections)
    {
      if (CanReachNode(pConnection->GetTargetPin().GetParent(), pTarget, Visited))
        return true;
    }
  }

  return false;
}


bool plDocumentNodeManager::WouldConnectionCreateCircle(const plPin& source, const plPin& target) const
{
  const plDocumentObject* pSourceNode = source.GetParent();
  const plDocumentObject* pTargetNode = target.GetParent();
  plSet<const plDocumentObject*> Visited;

  return CanReachNode(pTargetNode, pSourceNode, Visited);
}

void plDocumentNodeManager::GetDynamicPinNames(const plDocumentObject* pObject, plStringView sPropertyName, plStringView sPinName, plDynamicArray<plString>& out_Names) const
{
  out_Names.Clear();

  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(sPropertyName);
  if (pProp == nullptr)
  {
    plLog::Warning("Property '{0}' not found in type '{1}'", sPropertyName, pObject->GetType()->GetTypeName());
    return;
  }

  plStringBuilder sTemp;
  plVariant value = pObject->GetTypeAccessor().GetValue(sPropertyName);

  if (pProp->GetCategory() == plPropertyCategory::Member)
  {
    if (value.CanConvertTo<plUInt32>())
    {
      plUInt32 uiCount = value.ConvertTo<plUInt32>();
      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.Format("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
  else if (pProp->GetCategory() == plPropertyCategory::Array)
  {
    auto pArrayProp = static_cast<const plAbstractArrayProperty*>(pProp);

    auto& a = value.Get<plVariantArray>();
    const plUInt32 uiCount = a.GetCount();

    auto variantType = pArrayProp->GetSpecificType()->GetVariantType();
    if (variantType >= plVariantType::Int8 && variantType <= plVariantType::UInt64)
    {
      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.Format("{}", a[i]);
        out_Names.PushBack(sTemp);
      }
    }
    else if (variantType == plVariantType::String || variantType == plVariantType::HashedString)
    {
      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        out_Names.PushBack(a[i].ConvertTo<plString>());
      }
    }
    else
    {
      for (plUInt32 i = 0; i < uiCount; ++i)
      {
        sTemp.Format("{}[{}]", sPinName, i);
        out_Names.PushBack(sTemp);
      }
    }
  }
}

bool plDocumentNodeManager::TryRecreatePins(const plDocumentObject* pObject)
{
  if (!IsNode(pObject))
    return false;

  auto& nodeInternal = m_ObjectToNode[pObject->GetGuid()];

  for (auto& pPin : nodeInternal.m_Inputs)
  {
    if (HasConnections(*pPin))
      return false;
  }

  for (auto& pPin : nodeInternal.m_Outputs)
  {
    if (HasConnections(*pPin))
      return false;
  }

  {
    plDocumentNodeManagerEvent e(plDocumentNodeManagerEvent::Type::BeforePinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  nodeInternal.m_Inputs.Clear();
  nodeInternal.m_Outputs.Clear();
  InternalCreatePins(pObject, nodeInternal);

  {
    plDocumentNodeManagerEvent e(plDocumentNodeManagerEvent::Type::AfterPinsChanged, pObject);
    m_NodeEvents.Broadcast(e);
  }

  return true;
}

bool plDocumentNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  return true;
}

bool plDocumentNodeManager::InternalIsConnection(const plDocumentObject* pObject) const
{
  auto pType = pObject->GetTypeAccessor().GetType();
  return pType->IsDerivedFrom(GetConnectionType());
}

plStatus plDocumentNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_Result) const
{
  out_Result = CanConnectResult::ConnectNtoN;
  return plStatus(PLASMA_SUCCESS);
}

void plDocumentNodeManager::ObjectHandler(const plDocumentObjectEvent& e)
{
  switch (e.m_EventType)
  {
    case plDocumentObjectEvent::Type::AfterObjectCreated:
    {
      if (IsNode(e.m_pObject))
      {
        PLASMA_ASSERT_DEBUG(!m_ObjectToNode.Contains(e.m_pObject->GetGuid()), "Sanity check failed!");
        m_ObjectToNode[e.m_pObject->GetGuid()] = NodeInternal();
      }
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are created in Connect method.
      }
    }
    break;
    case plDocumentObjectEvent::Type::BeforeObjectDestroyed:
    {
      if (IsNode(e.m_pObject))
      {
        auto it = m_ObjectToNode.Find(e.m_pObject->GetGuid());
        PLASMA_ASSERT_DEBUG(it.IsValid(), "Sanity check failed!");

        m_ObjectToNode.Remove(it);
      }
      else if (IsConnection(e.m_pObject))
      {
        // Nothing to do here: Map entries are removed in Disconnect method.
      }
    }
    break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED
  }
}

void plDocumentNodeManager::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        auto& nodeInternal = m_ObjectToNode[e.m_pObject->GetGuid()];
        if (nodeInternal.m_Inputs.IsEmpty() && nodeInternal.m_Outputs.IsEmpty())
        {
          InternalCreatePins(e.m_pObject, nodeInternal);
          // TODO: Sanity check pins (duplicate names etc).
        }

        plDocumentNodeManagerEvent e2(plDocumentNodeManagerEvent::Type::BeforeNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      if (IsNode(e.m_pObject))
      {
        plDocumentNodeManagerEvent e2(plDocumentNodeManagerEvent::Type::AfterNodeAdded, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case plDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        plDocumentNodeManagerEvent e2(plDocumentNodeManagerEvent::Type::BeforeNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      if (IsNode(e.m_pObject))
      {
        plDocumentNodeManagerEvent e2(plDocumentNodeManagerEvent::Type::AfterNodeRemoved, e.m_pObject);
        m_NodeEvents.Broadcast(e2);
      }
    }
    break;

    default:
      break;
  }
}

void plDocumentNodeManager::PropertyEventsHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_pObject == nullptr)
    return;

  const plAbstractProperty* pProp = e.m_pObject->GetType()->FindPropertyByName(e.m_sProperty);
  if (pProp == nullptr)
    return;

  if (IsDynamicPinProperty(e.m_pObject, pProp))
  {
    TryRecreatePins(e.m_pObject);
  }
}

void plDocumentNodeManager::RestoreOldMetaDataAfterLoading(const plAbstractObjectGraph& graph, const plAbstractObjectNode::Property& connectionsProperty, const plDocumentObject* pSourceObject)
{
  if (connectionsProperty.m_Value.IsA<plVariantArray>() == false)
    return;

  const plVariantArray& array = connectionsProperty.m_Value.Get<plVariantArray>();
  for (const plVariant& var : array)
  {
    if (var.IsA<plUuid>() == false)
      continue;

    auto pOldConnectionAbstractObject = graph.GetNode(var.Get<plUuid>());
    auto pTargetProperty = pOldConnectionAbstractObject->FindProperty("Target");
    if (pTargetProperty == nullptr || pTargetProperty->m_Value.IsA<plUuid>() == false)
      continue;

    plDocumentObject* pTargetObject = GetObject(pTargetProperty->m_Value.Get<plUuid>());
    if (pTargetObject == nullptr)
      continue;

    auto pSourcePinProperty = pOldConnectionAbstractObject->FindProperty("SourcePin");
    if (pSourcePinProperty == nullptr || pSourcePinProperty->m_Value.IsA<plString>() == false)
      continue;

    auto pTargetPinProperty = pOldConnectionAbstractObject->FindProperty("TargetPin");
    if (pTargetPinProperty == nullptr || pTargetPinProperty->m_Value.IsA<plString>() == false)
      continue;

    const plPin* pSourcePin = GetOutputPinByName(pSourceObject, pSourcePinProperty->m_Value.Get<plString>());
    const plPin* pTargetPin = GetInputPinByName(pTargetObject, pTargetPinProperty->m_Value.Get<plString>());
    if (pSourcePin == nullptr || pTargetPin == nullptr)
      continue;

    const plRTTI* pConnectionType = GetConnectionType();
    plDocumentNodeManager::CanConnectResult res;
    if (CanConnect(pConnectionType, *pSourcePin, *pTargetPin, res).m_Result.Succeeded())
    {
      plDocumentObject* pConnectionObject = CreateObject(pConnectionType, plUuid::MakeUuid());

      AddObject(pConnectionObject, nullptr, "", -1);

      Connect(pConnectionObject, *pSourcePin, *pTargetPin);
    }
  }
}
