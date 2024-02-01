#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptGraph.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualScriptPin, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plVisualScriptPin::plVisualScriptPin(Type type, plStringView sName, const plVisualScriptNodeRegistry::PinDesc& pinDesc, const plDocumentObject* pObject, plUInt32 uiDataPinIndex, plUInt32 uiElementIndex)
  : plPin(type, sName, pinDesc.GetColor(), pObject)
  , m_pDesc(&pinDesc)
  , m_uiDataPinIndex(uiDataPinIndex)
  , m_uiElementIndex(uiElementIndex)
{
  if (pinDesc.IsExecutionPin())
  {
    m_Shape = Shape::Arrow;
  }
  else
  {
    m_Shape = (pinDesc.m_ScriptDataType == plVisualScriptDataType::Array || pinDesc.m_ScriptDataType == plVisualScriptDataType::Map) ? Shape::Rect : Shape::Circle;
  }
}

plVisualScriptPin::~plVisualScriptPin()
{
  auto pManager = static_cast<plVisualScriptNodeManager*>(const_cast<plDocumentObjectManager*>(GetParent()->GetDocumentObjectManager()));
  pManager->RemoveDeductedPinType(*this);
}

plVisualScriptDataType::Enum plVisualScriptPin::GetResolvedScriptDataType() const
{
  auto scriptDataType = GetScriptDataType();
  if (scriptDataType == plVisualScriptDataType::AnyPointer || scriptDataType == plVisualScriptDataType::Any)
  {
    auto pManager = static_cast<const plVisualScriptNodeManager*>(GetParent()->GetDocumentObjectManager());
    return pManager->GetDeductedType(*this);
  }

  return scriptDataType;
}

plStringView plVisualScriptPin::GetDataTypeName() const
{
  plVisualScriptDataType::Enum resolvedDataType = GetResolvedScriptDataType();
  if (resolvedDataType == plVisualScriptDataType::Invalid)
  {
    return plVisualScriptDataType::GetName(GetScriptDataType());
  }

  if ((resolvedDataType == plVisualScriptDataType::TypedPointer || resolvedDataType == plVisualScriptDataType::EnumValue) && GetDataType() != nullptr)
  {
    return GetDataType()->GetTypeName();
  }

  return plVisualScriptDataType::GetName(resolvedDataType);
}

bool plVisualScriptPin::CanConvertTo(const plVisualScriptPin& targetPin, bool bUseResolvedDataTypes /*= true*/) const
{
  plVisualScriptDataType::Enum sourceScriptDataType = bUseResolvedDataTypes ? GetResolvedScriptDataType() : GetScriptDataType();
  plVisualScriptDataType::Enum targetScriptDataType = bUseResolvedDataTypes ? targetPin.GetResolvedScriptDataType() : targetPin.GetScriptDataType();

  const plRTTI* pSourceDataType = GetDataType();
  const plRTTI* pTargetDataType = targetPin.GetDataType();

  if (plVisualScriptDataType::IsPointer(sourceScriptDataType) &&
      targetScriptDataType == plVisualScriptDataType::AnyPointer)
    return true;

  if (sourceScriptDataType == plVisualScriptDataType::TypedPointer && pSourceDataType != nullptr &&
      targetScriptDataType == plVisualScriptDataType::TypedPointer && pTargetDataType != nullptr)
    return pSourceDataType->IsDerivedFrom(pTargetDataType);

  if (sourceScriptDataType == plVisualScriptDataType::EnumValue && pSourceDataType != nullptr &&
      targetScriptDataType == plVisualScriptDataType::EnumValue && pTargetDataType != nullptr)
    return pSourceDataType == pTargetDataType;

  if (sourceScriptDataType == plVisualScriptDataType::Any ||
      targetScriptDataType == plVisualScriptDataType::Any)
    return true;

  return plVisualScriptDataType::CanConvertTo(sourceScriptDataType, targetScriptDataType);
}

//////////////////////////////////////////////////////////////////////////

plVisualScriptNodeManager::plVisualScriptNodeManager()
{
  m_NodeEvents.AddEventHandler(plMakeDelegate(&plVisualScriptNodeManager::NodeEventsHandler, this));
  m_PropertyEvents.AddEventHandler(plMakeDelegate(&plVisualScriptNodeManager::PropertyEventsHandler, this));
}

plVisualScriptNodeManager::~plVisualScriptNodeManager() = default;

plHashedString plVisualScriptNodeManager::GetScriptBaseClass() const
{
  plHashedString sBaseClass;
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    plVariant baseClass = GetRootObject()->GetChildren()[0]->GetTypeAccessor().GetValue("BaseClass");
    if (baseClass.IsA<plString>())
    {
      sBaseClass.Assign(baseClass.Get<plString>());
    }
  }
  return sBaseClass;
}

bool plVisualScriptNodeManager::IsFilteredByBaseClass(const plRTTI* pNodeType, const plVisualScriptNodeRegistry::NodeDesc& nodeDesc, const plHashedString& sBaseClass, bool bLogWarning /*= false*/) const
{
  if (nodeDesc.m_sFilterByBaseClass.IsEmpty() == false && nodeDesc.m_sFilterByBaseClass != sBaseClass)
  {
    if (bLogWarning)
    {
      plStringView sTypeName = pNodeType->GetTypeName();
      sTypeName.TrimWordStart(plVisualScriptNodeRegistry::s_szTypeNamePrefix);

      plLog::Warning("The base class function '{}' is not a function of the currently selected base class '{}' and will be skipped", sTypeName, sBaseClass);
    }

    return true;
  }

  return false;
}


plVisualScriptDataType::Enum plVisualScriptNodeManager::GetVariableType(plTempHashedString sName) const
{
  plVariant defaultValue;
  GetVariableDefaultValue(sName, defaultValue).IgnoreResult();
  return plVisualScriptDataType::FromVariantType(defaultValue.GetType());
}

plResult plVisualScriptNodeManager::GetVariableDefaultValue(plTempHashedString sName, plVariant& out_value) const
{
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    auto& typeAccessor = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    plUInt32 uiNumVariables = typeAccessor.GetCount("Variables");
    for (plUInt32 i = 0; i < uiNumVariables; ++i)
    {
      plVariant variableUuid = typeAccessor.GetValue("Variables", i);
      if (variableUuid.IsA<plUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<plUuid>());
      if (pVariableObject == nullptr)
        continue;

      plVariant nameVar = pVariableObject->GetTypeAccessor().GetValue("Name");
      if (nameVar.IsA<plHashedString>() == false || nameVar.Get<plHashedString>() != sName)
        continue;

      out_value = pVariableObject->GetTypeAccessor().GetValue("DefaultValue");
      return PL_SUCCESS;
    }
  }

  return PL_FAILURE;
}

void plVisualScriptNodeManager::GetInputExecutionPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = plStaticCast<const plVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void plVisualScriptNodeManager::GetOutputExecutionPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = plStaticCast<const plVisualScriptPin&>(*pPin);
    if (vsPin.IsExecutionPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void plVisualScriptNodeManager::GetInputDataPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetInputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = plStaticCast<const plVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void plVisualScriptNodeManager::GetOutputDataPins(const plDocumentObject* pObject, plDynamicArray<const plVisualScriptPin*>& out_pins) const
{
  out_pins.Clear();

  auto pins = GetOutputPins(pObject);
  for (auto& pPin : pins)
  {
    auto& vsPin = plStaticCast<const plVisualScriptPin&>(*pPin);
    if (vsPin.IsDataPin())
    {
      out_pins.PushBack(&vsPin);
    }
  }
}

void plVisualScriptNodeManager::GetEntryNodes(const plDocumentObject* pObject, plDynamicArray<const plDocumentObject*>& out_entryNodes) const
{
  plHybridArray<const plDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pObject);

  plHashSet<const plDocumentObject*> visitedNodes;
  plHybridArray<const plVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const plDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (plVisualScriptNodeDescription::Type::IsEntry(pNodeDesc->m_Type))
    {
      out_entryNodes.PushBack(pCurrentNode);
      continue;
    }

    GetInputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        const plDocumentObject* pSourceNode = pConnection->GetSourcePin().GetParent();
        if (visitedNodes.Insert(pSourceNode))
          continue;

        nodeStack.PushBack(pSourceNode);
      }
    }
  }
}

// static
plStringView plVisualScriptNodeManager::GetNiceTypeName(const plDocumentObject* pObject)
{
  plStringView sTypeName = pObject->GetType()->GetTypeName();

  while (sTypeName.TrimWordStart(plVisualScriptNodeRegistry::s_szTypeNamePrefix) ||
         sTypeName.TrimWordStart("Builtin_"))
  {
  }

  if (const char* szAngleBracket = sTypeName.FindSubString("<"))
  {
    sTypeName = plStringView(sTypeName.GetStartPointer(), szAngleBracket);
  }

  return sTypeName;
}

plStringView plVisualScriptNodeManager::GetNiceFunctionName(const plDocumentObject* pObject)
{
  plStringView sFunctionName = pObject->GetType()->GetTypeName();

  if (const char* szSeparator = sFunctionName.FindLastSubString("::"))
  {
    sFunctionName = plStringView(szSeparator + 2, sFunctionName.GetEndPointer());
  }

  return sFunctionName;
}

plVisualScriptDataType::Enum plVisualScriptNodeManager::GetDeductedType(const plVisualScriptPin& pin) const
{
  plEnum<plVisualScriptDataType> dataType = plVisualScriptDataType::Invalid;
  m_PinToDeductedType.TryGetValue(&pin, dataType);
  return dataType;
}

plVisualScriptDataType::Enum plVisualScriptNodeManager::GetDeductedType(const plDocumentObject* pObject) const
{
  plEnum<plVisualScriptDataType> dataType = plVisualScriptDataType::Invalid;
  m_ObjectToDeductedType.TryGetValue(pObject, dataType);
  return dataType;
}

bool plVisualScriptNodeManager::IsCoroutine(const plDocumentObject* pObject) const
{
  auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && plVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
  {
    return true;
  }

  return m_CoroutineObjects.Contains(pObject);
}

bool plVisualScriptNodeManager::IsLoop(const plDocumentObject* pObject) const
{
  auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc != nullptr && plVisualScriptNodeDescription::Type::IsLoop(pNodeDesc->m_Type))
  {
    return true;
  }

  return false;
}

bool plVisualScriptNodeManager::InternalIsNode(const plDocumentObject* pObject) const
{
  return pObject->GetType()->IsDerivedFrom(plVisualScriptNodeRegistry::GetSingleton()->GetNodeBaseType());
}

bool plVisualScriptNodeManager::InternalIsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const
{
  auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc != nullptr && pNodeDesc->m_bHasDynamicPins)
  {
    plTempHashedString sPropNameHashed = plTempHashedString(pProp->GetPropertyName());
    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }

    for (auto& pinDesc : pNodeDesc->m_OutputPins)
    {
      if (pinDesc.m_sDynamicPinProperty == sPropNameHashed)
      {
        return true;
      }
    }
  }

  return false;
}

plStatus plVisualScriptNodeManager::InternalCanConnect(const plPin& source, const plPin& target, CanConnectResult& out_result) const
{
  const plVisualScriptPin& pinSource = plStaticCast<const plVisualScriptPin&>(source);
  const plVisualScriptPin& pinTarget = plStaticCast<const plVisualScriptPin&>(target);

  if (pinSource.IsExecutionPin() != pinTarget.IsExecutionPin())
  {
    out_result = CanConnectResult::ConnectNever;
    return plStatus("Cannot connect data pins with execution pins.");
  }

  if (pinSource.IsDataPin() && pinSource.CanConvertTo(pinTarget, false) == false)
  {
    out_result = CanConnectResult::ConnectNever;
    return plStatus(plFmt("The pin data types are incompatible."));
  }

  if (WouldConnectionCreateCircle(source, target))
  {
    out_result = CanConnectResult::ConnectNever;
    return plStatus("Connecting these pins would create a circle in the graph.");
  }

  // only one connection is allowed on DATA input pins, execution input pins may have multiple incoming connections
  if (pinTarget.IsDataPin() && HasConnections(pinTarget))
  {
    out_result = CanConnectResult::ConnectNto1;
    return plStatus(PL_FAILURE);
  }

  // only one outgoing connection is allowed on EXECUTION pins, data pins may have multiple outgoing connections
  if (pinSource.IsExecutionPin() && HasConnections(pinSource))
  {
    out_result = CanConnectResult::Connect1toN;
    return plStatus(PL_FAILURE);
  }

  out_result = CanConnectResult::ConnectNtoN;
  return plStatus(PL_SUCCESS);
}

void plVisualScriptNodeManager::InternalCreatePins(const plDocumentObject* pObject, NodeInternal& ref_node)
{
  auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());

  if (pNodeDesc == nullptr)
    return;

  plHybridArray<plString, 16> dynamicPinNames;
  auto CreatePins = [&](const plVisualScriptNodeRegistry::PinDesc& pinDesc, plPin::Type type, plDynamicArray<plUniquePtr<plPin>>& out_pins, plUInt32& inout_dataPinIndex)
  {
    if (pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
    {
      GetDynamicPinNames(pObject, pinDesc.m_sDynamicPinProperty, pinDesc.m_sName, dynamicPinNames);
    }
    else
    {
      dynamicPinNames.Clear();
      dynamicPinNames.PushBack(pinDesc.m_sName.GetView());
    }

    for (plUInt32 i = 0; i < dynamicPinNames.GetCount(); ++i)
    {
      plUInt32 uiDataPinIndex = plInvalidIndex;
      if (pinDesc.IsDataPin())
      {
        uiDataPinIndex = inout_dataPinIndex;
        ++inout_dataPinIndex;
      }

      auto pPin = PL_DEFAULT_NEW(plVisualScriptPin, type, dynamicPinNames[i], pinDesc, pObject, uiDataPinIndex, i);
      out_pins.PushBack(pPin);
    }
  };

  plUInt32 uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_InputPins)
  {
    CreatePins(pinDesc, plPin::Type::Input, ref_node.m_Inputs, uiDataPinIndex);
  }

  uiDataPinIndex = 0;
  for (const auto& pinDesc : pNodeDesc->m_OutputPins)
  {
    CreatePins(pinDesc, plPin::Type::Output, ref_node.m_Outputs, uiDataPinIndex);
  }
}

void plVisualScriptNodeManager::GetNodeCreationTemplates(plDynamicArray<plNodeCreationTemplate>& out_templates) const
{
  auto pRegistry = plVisualScriptNodeRegistry::GetSingleton();
  auto propertyValues = pRegistry->GetPropertyValues();
  plHashedString sBaseClass = GetScriptBaseClass();

  for (auto& nodeTemplate : pRegistry->GetNodeCreationTemplates())
  {
    const plRTTI* pNodeType = nodeTemplate.m_pType;

    if (IsFilteredByBaseClass(pNodeType, *pRegistry->GetNodeDescForType(pNodeType), sBaseClass))
      continue;

    if (!pNodeType->GetTypeFlags().IsSet(plTypeFlags::Abstract))
    {
      auto& temp = out_templates.ExpandAndGetRef();
      temp.m_pType = pNodeType;
      temp.m_sTypeName = nodeTemplate.m_sTypeName;
      temp.m_sCategory = nodeTemplate.m_sCategory;
      temp.m_PropertyValues = propertyValues.GetSubArray(nodeTemplate.m_uiPropertyValuesStart, nodeTemplate.m_uiPropertyValuesCount);
    }
  }

  // Getter and setter templates for variables
  if (GetRootObject()->GetChildren().IsEmpty() == false)
  {
    static plHashedString sVariables = plMakeHashedString("Variables");
    static plHashedString sName = plMakeHashedString("Name");

    m_PropertyValues.Clear();
    m_VariableNodeTypeNames.Clear();

    plStringBuilder sNodeTypeName;

    auto& typeAccessor = GetRootObject()->GetChildren()[0]->GetTypeAccessor();
    const plUInt32 uiNumVariables = typeAccessor.GetCount(sVariables.GetView());
    for (plUInt32 i = 0; i < uiNumVariables; ++i)
    {
      plVariant variableUuid = typeAccessor.GetValue(sVariables.GetView(), i);
      if (variableUuid.IsA<plUuid>() == false)
        continue;

      auto pVariableObject = GetObject(variableUuid.Get<plUuid>());
      if (pVariableObject == nullptr)
        continue;

      plVariant nameVar = pVariableObject->GetTypeAccessor().GetValue(sName.GetView());
      if (nameVar.IsA<plHashedString>() == false)
        continue;

      plHashedString sVariableName = nameVar.Get<plHashedString>();

      plUInt32 uiStart = m_PropertyValues.GetCount();
      m_PropertyValues.PushBack({sName, nameVar});

      // Setter
      {
        sNodeTypeName.Set("Set", sVariableName);
        m_VariableNodeTypeNames.PushBack(sNodeTypeName);

        auto& temp = out_templates.ExpandAndGetRef();
        temp.m_pType = pRegistry->GetVariableSetterType();
        temp.m_sTypeName = m_VariableNodeTypeNames.PeekBack();
        temp.m_sCategory = sVariables;
        temp.m_PropertyValues = m_PropertyValues.GetArrayPtr().GetSubArray(uiStart, 1);
      }

      // Getter
      {
        sNodeTypeName.Set("Get", sVariableName);
        m_VariableNodeTypeNames.PushBack(sNodeTypeName);

        auto& temp = out_templates.ExpandAndGetRef();
        temp.m_pType = pRegistry->GetVariableGetterType();
        temp.m_sTypeName = m_VariableNodeTypeNames.PeekBack();
        temp.m_sCategory = sVariables;
        temp.m_PropertyValues = m_PropertyValues.GetArrayPtr().GetSubArray(uiStart, 1);
      }
    }
  }
}

void plVisualScriptNodeManager::NodeEventsHandler(const plDocumentNodeManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case plDocumentNodeManagerEvent::Type::AfterPinsConnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent());
      UpdateCoroutine(targetPin.GetParent(), connection);
    }
    break;

    case plDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
    {
      auto& connection = GetConnection(e.m_pObject);
      auto& targetPin = connection.GetTargetPin();
      DeductNodeTypeAndAllPinTypes(targetPin.GetParent(), &targetPin);
      UpdateCoroutine(targetPin.GetParent(), connection, false);
    }
    break;

    case plDocumentNodeManagerEvent::Type::AfterNodeAdded:
    {
      DeductNodeTypeAndAllPinTypes(e.m_pObject);
    }
    break;

    case plDocumentNodeManagerEvent::Type::BeforeNodeRemoved:
    {
      m_ObjectToDeductedType.Remove(e.m_pObject);
    }
    break;

    default:
      break;
  }
}

void plVisualScriptNodeManager::PropertyEventsHandler(const plDocumentObjectPropertyEvent& e)
{
  if (IsNode(e.m_pObject))
  {
    DeductNodeTypeAndAllPinTypes(e.m_pObject);
  }
  else if (e.m_sProperty == "Name" || e.m_sProperty == "DefaultValue") // a variable's name or default value has changed, re-run type deduction
  {
    for (auto pObject : GetRootObject()->GetChildren())
    {
      if (IsNode(pObject) == false)
        continue;

      DeductNodeTypeAndAllPinTypes(pObject);
    }
  }
}

void plVisualScriptNodeManager::RemoveDeductedPinType(const plVisualScriptPin& pin)
{
  m_PinToDeductedType.Remove(&pin);
}

void plVisualScriptNodeManager::DeductNodeTypeAndAllPinTypes(const plDocumentObject* pObject, const plPin* pDisconnectedPin /*= nullptr*/)
{
  auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
  if (pNodeDesc == nullptr || pNodeDesc->NeedsTypeDeduction() == false)
    return;

  if (pDisconnectedPin != nullptr && static_cast<const plVisualScriptPin*>(pDisconnectedPin)->NeedsTypeDeduction() == false)
    return;

  bool bNodeTypeChanged = false;
  {
    plEnum<plVisualScriptDataType> newDeductedType = pNodeDesc->m_DeductTypeFunc(pObject, static_cast<const plVisualScriptPin*>(pDisconnectedPin));
    plEnum<plVisualScriptDataType> oldDeductedType = plVisualScriptDataType::Invalid;
    m_ObjectToDeductedType.Insert(pObject, newDeductedType, &oldDeductedType);

    bNodeTypeChanged = (newDeductedType != oldDeductedType);
  }

  bool bAnyInputPinChanged = false;
  plHybridArray<const plVisualScriptPin*, 16> pins;
  GetInputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      plEnum<plVisualScriptDataType> newDeductedType = pFunc(*pPin);
      plEnum<plVisualScriptDataType> oldDeductedType = plVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyInputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  bool bAnyOutputPinChanged = false;
  GetOutputDataPins(pObject, pins);
  for (auto pPin : pins)
  {
    if (auto pFunc = pPin->GetDeductTypeFunc())
    {
      plEnum<plVisualScriptDataType> newDeductedType = pFunc(*pPin);
      plEnum<plVisualScriptDataType> oldDeductedType = plVisualScriptDataType::Invalid;
      m_PinToDeductedType.Insert(pPin, newDeductedType, &oldDeductedType);

      bAnyOutputPinChanged |= (newDeductedType != oldDeductedType);
    }
  }

  if (bNodeTypeChanged || bAnyInputPinChanged || bAnyOutputPinChanged)
  {
    m_NodeChangedEvent.Broadcast(pObject);
  }

  // propagate to connected nodes
  if (bAnyOutputPinChanged)
  {
    for (auto pPin : pins)
    {
      if (pPin->NeedsTypeDeduction() == false)
        continue;

      auto connections = GetConnections(*pPin);
      for (auto& connection : connections)
      {
        DeductNodeTypeAndAllPinTypes(connection->GetTargetPin().GetParent());
      }
    }
  }
}

void plVisualScriptNodeManager::UpdateCoroutine(const plDocumentObject* pTargetNode, const plConnection& changedConnection, bool bIsAboutToDisconnect)
{
  auto vsPin = static_cast<const plVisualScriptPin&>(changedConnection.GetTargetPin());
  if (vsPin.IsExecutionPin() == false)
    return;

  plHybridArray<const plDocumentObject*, 16> entryNodes;
  GetEntryNodes(pTargetNode, entryNodes);

  for (auto pEntryNode : entryNodes)
  {
    const bool bWasCoroutine = m_CoroutineObjects.Contains(pEntryNode);
    const bool bIsCoroutine = IsConnectedToCoroutine(pEntryNode, changedConnection, bIsAboutToDisconnect);

    if (bWasCoroutine != bIsCoroutine)
    {
      if (bIsCoroutine)
      {
        m_CoroutineObjects.Insert(pEntryNode);
      }
      else
      {
        m_CoroutineObjects.Remove(pEntryNode);
      }

      m_NodeChangedEvent.Broadcast(pEntryNode);
    }
  }
}

bool plVisualScriptNodeManager::IsConnectedToCoroutine(const plDocumentObject* pEntryNode, const plConnection& changedConnection, bool bIsAboutToDisconnect) const
{
  plHybridArray<const plDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryNode);

  plHashSet<const plDocumentObject*> visitedNodes;
  plHybridArray<const plVisualScriptPin*, 16> pins;

  while (nodeStack.IsEmpty() == false)
  {
    const plDocumentObject* pCurrentNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pCurrentNode->GetType());
    if (plVisualScriptNodeDescription::Type::MakesOuterCoroutine(pNodeDesc->m_Type))
    {
      return true;
    }

    GetOutputExecutionPins(pCurrentNode, pins);
    for (auto pPin : pins)
    {
      if (pPin->SplitExecution())
        continue;

      auto connections = GetConnections(*pPin);
      for (auto pConnection : connections)
      {
        // the connection is about to be disconnected so we ignore it here
        if (bIsAboutToDisconnect && pConnection == &changedConnection)
          continue;

        const plDocumentObject* pTargetNode = pConnection->GetTargetPin().GetParent();
        if (visitedNodes.Insert(pTargetNode))
          continue;

        nodeStack.PushBack(pTargetNode);
      }
    }
  }

  return false;
}
