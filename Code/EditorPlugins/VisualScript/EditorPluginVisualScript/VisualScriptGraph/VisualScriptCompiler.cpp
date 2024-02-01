#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/World/World.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptCompiler.h>
#include <EditorPluginVisualScript/VisualScriptGraph/VisualScriptTypeDeduction.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/SimdMath/SimdRandom.h>
#include <Foundation/Utilities/DGMLWriter.h>

namespace
{
  void MakeSubfunctionName(const plDocumentObject* pObject, const plDocumentObject* pEntryObject, plStringBuilder& out_sName)
  {
    plVariant sNameProperty = pObject->GetTypeAccessor().GetValue("Name");
    plUInt32 uiHash = plHashHelper<plUuid>::Hash(pObject->GetGuid());

    out_sName.SetFormat("{}_{}_{}", pEntryObject != nullptr ? plVisualScriptNodeManager::GetNiceFunctionName(pEntryObject) : "", sNameProperty, plArgU(uiHash, 8, true, 16));
  }

  plVisualScriptDataType::Enum FinalizeDataType(plVisualScriptDataType::Enum dataType)
  {
    plVisualScriptDataType::Enum result = dataType;
    if (result == plVisualScriptDataType::EnumValue)
      result = plVisualScriptDataType::Int64;

    return result;
  }

  using FillUserDataFunction = plResult (*)(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject);

  static plResult FillUserData_CoroutineMode(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("CoroutineMode");
    return PL_SUCCESS;
  }

  static plResult FillUserData_ReflectedPropertyOrFunction(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
      inout_astNode.m_sTargetTypeName.Assign(pNodeDesc->m_pTargetType->GetTypeName());

    plVariantArray propertyNames;
    for (auto& pProp : pNodeDesc->m_TargetProperties)
    {
      plHashedString sPropertyName;
      sPropertyName.Assign(pProp->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return PL_SUCCESS;
  }

  static plResult FillUserData_DynamicReflectedProperty(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    auto pTargetType = plVisualScriptTypeDeduction::GetReflectedType(pObject);
    auto pTargetProperty = plVisualScriptTypeDeduction::GetReflectedProperty(pObject);
    if (pTargetType == nullptr || pTargetProperty == nullptr)
      return PL_FAILURE;

    inout_astNode.m_sTargetTypeName.Assign(pTargetType->GetTypeName());

    plVariantArray propertyNames;
    {
      plHashedString sPropertyName;
      sPropertyName.Assign(pTargetProperty->GetPropertyName());
      propertyNames.PushBack(sPropertyName);
    }

    inout_astNode.m_Value = propertyNames;

    return PL_SUCCESS;
  }

  static plResult FillUserData_ConstantValue(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Value");
    inout_astNode.m_DeductedDataType = plVisualScriptDataType::FromVariantType(inout_astNode.m_Value.GetType());
    return PL_SUCCESS;
  }

  static plResult FillUserData_VariableName(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Name");

    plStringView sName = inout_astNode.m_Value.Get<plString>().GetView();

    plVariant defaultValue;
    if (static_cast<const plVisualScriptNodeManager*>(pObject->GetDocumentObjectManager())->GetVariableDefaultValue(plTempHashedString(sName), defaultValue).Failed())
    {
      plLog::Error("Invalid variable named '{}'", sName);
      return PL_FAILURE;
    }

    return PL_SUCCESS;
  }

  static plResult FillUserData_Switch(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    inout_astNode.m_DeductedDataType = plVisualScriptDataType::Int64;

    plVariantArray casesVarArray;

    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    if (pNodeDesc->m_pTargetType != nullptr)
    {
      plHybridArray<plReflectionUtils::EnumKeyValuePair, 16> enumKeysAndValues;
      plReflectionUtils::GetEnumKeysAndValues(pNodeDesc->m_pTargetType, enumKeysAndValues, plReflectionUtils::EnumConversionMode::ValueNameOnly);
      for (auto& keyAndValue : enumKeysAndValues)
      {
        casesVarArray.PushBack(keyAndValue.m_iValue);
      }
    }
    else
    {
      plVariant casesVar = pObject->GetTypeAccessor().GetValue("Cases");
      casesVarArray = casesVar.Get<plVariantArray>();
      for (auto& caseVar : casesVarArray)
      {
        if (caseVar.IsA<plString>())
        {
          inout_astNode.m_DeductedDataType = plVisualScriptDataType::HashedString;
          caseVar = plTempHashedString(caseVar.Get<plString>()).GetHash();
        }
        else if (caseVar.IsA<plHashedString>())
        {
          inout_astNode.m_DeductedDataType = plVisualScriptDataType::HashedString;
          caseVar = caseVar.Get<plHashedString>().GetHash();
        }
      }
    }

    inout_astNode.m_Value = casesVarArray;
    return PL_SUCCESS;
  }

  static plResult FillUserData_Builtin_Compare(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    inout_astNode.m_Value = pObject->GetTypeAccessor().GetValue("Operator");
    return PL_SUCCESS;
  }

  static plResult FillUserData_Builtin_Expression(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    auto pManager = static_cast<const plVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());

    plHybridArray<const plVisualScriptPin*, 16> pins;

    plHybridArray<plExpression::StreamDesc, 8> inputs;
    pManager->GetInputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& input = inputs.ExpandAndGetRef();
      input.m_sName.Assign(pPin->GetName());
      input.m_DataType = plVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    plHybridArray<plExpression::StreamDesc, 8> outputs;
    pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& output = outputs.ExpandAndGetRef();
      output.m_sName.Assign(pPin->GetName());
      output.m_DataType = plVisualScriptDataType::GetStreamDataType(pPin->GetResolvedScriptDataType());
    }

    plString sExpressionSource = pObject->GetTypeAccessor().GetValue("Expression").Get<plString>();

    plExpressionParser parser;
    plExpressionParser::Options options = {};
    plExpressionAST ast;
    PL_SUCCEED_OR_RETURN(parser.Parse(sExpressionSource, inputs, outputs, options, ast));

    plExpressionCompiler compiler;
    plExpressionByteCode byteCode;
    PL_SUCCEED_OR_RETURN(compiler.Compile(ast, byteCode));

    inout_astNode.m_Value = byteCode;

    return PL_SUCCESS;
  }

  static plResult FillUserData_Builtin_TryGetComponentOfBaseType(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    auto typeName = pObject->GetTypeAccessor().GetValue("TypeName");
    const plRTTI* pType = plRTTI::FindTypeByName(typeName.Get<plString>());
    if (pType == nullptr)
    {
      plLog::Error("Invalid type '{}' for GameObject::TryGetComponentOfBaseType node.", typeName);
      return PL_FAILURE;
    }

    inout_astNode.m_sTargetTypeName.Assign(pType->GetTypeName());
    return PL_SUCCESS;
  }

  static plResult FillUserData_Builtin_StartCoroutine(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    PL_SUCCEED_OR_RETURN(FillUserData_CoroutineMode(inout_astNode, pCompiler, pObject, pEntryObject));

    auto pManager = static_cast<const plVisualScriptNodeManager*>(pObject->GetDocumentObjectManager());
    plHybridArray<const plVisualScriptPin*, 16> pins;
    pManager->GetOutputExecutionPins(pObject, pins);

    const plUInt32 uiCoroutineBodyIndex = 1;
    auto connections = pManager->GetConnections(*pins[uiCoroutineBodyIndex]);
    if (connections.IsEmpty() == false)
    {
      plStringBuilder sFunctionName;
      MakeSubfunctionName(pObject, pEntryObject, sFunctionName);

      plStringBuilder sFullName;
      sFullName.Set(pCompiler->GetCompiledModule().m_sScriptClassName, "::", sFunctionName, "<Coroutine>");

      inout_astNode.m_sTargetTypeName.Assign(sFullName);

      return pCompiler->AddFunction(sFunctionName, connections[0]->GetTargetPin().GetParent(), pObject);
    }

    return PL_SUCCESS;
  }

  static FillUserDataFunction s_TypeToFillUserDataFunctions[] = {
    nullptr,                                   // Invalid,
    &FillUserData_CoroutineMode,               // EntryCall,
    &FillUserData_CoroutineMode,               // EntryCall_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction, // MessageHandler,
    &FillUserData_ReflectedPropertyOrFunction, // MessageHandler_Coroutine,
    &FillUserData_ReflectedPropertyOrFunction, // ReflectedFunction,
    &FillUserData_DynamicReflectedProperty,    // GetReflectedProperty,
    &FillUserData_DynamicReflectedProperty,    // SetReflectedProperty,
    &FillUserData_ReflectedPropertyOrFunction, // InplaceCoroutine,
    nullptr,                                   // GetOwner,
    &FillUserData_ReflectedPropertyOrFunction, // SendMessage,

    nullptr, // FirstBuiltin,

    &FillUserData_ConstantValue, // Builtin_Constant,
    &FillUserData_VariableName,  // Builtin_GetVariable,
    &FillUserData_VariableName,  // Builtin_SetVariable,
    &FillUserData_VariableName,  // Builtin_IncVariable,
    &FillUserData_VariableName,  // Builtin_DecVariable,

    nullptr,              // Builtin_Branch,
    &FillUserData_Switch, // Builtin_Switch,
    nullptr,              // Builtin_WhileLoop,
    nullptr,              // Builtin_ForLoop,
    nullptr,              // Builtin_ForEachLoop,
    nullptr,              // Builtin_ReverseForEachLoop,
    nullptr,              // Builtin_Break,
    nullptr,              // Builtin_Jump,

    nullptr,                       // Builtin_And,
    nullptr,                       // Builtin_Or,
    nullptr,                       // Builtin_Not,
    &FillUserData_Builtin_Compare, // Builtin_Compare,
    &FillUserData_Builtin_Compare, // Builtin_CompareExec,
    nullptr,                       // Builtin_IsValid,
    nullptr,                       // Builtin_Select,

    nullptr,                          // Builtin_Add,
    nullptr,                          // Builtin_Subtract,
    nullptr,                          // Builtin_Multiply,
    nullptr,                          // Builtin_Divide,
    &FillUserData_Builtin_Expression, // Builtin_Expression,

    nullptr, // Builtin_ToBool,
    nullptr, // Builtin_ToByte,
    nullptr, // Builtin_ToInt,
    nullptr, // Builtin_ToInt64,
    nullptr, // Builtin_ToFloat,
    nullptr, // Builtin_ToDouble,
    nullptr, // Builtin_ToString,
    nullptr, // Builtin_String_Format,
    nullptr, // Builtin_ToHashedString,
    nullptr, // Builtin_ToVariant,
    nullptr, // Builtin_Variant_ConvertTo,

    nullptr, // Builtin_MakeArray
    nullptr, // Builtin_Array_GetElement,
    nullptr, // Builtin_Array_SetElement,
    nullptr, // Builtin_Array_GetCount,
    nullptr, // Builtin_Array_IsEmpty,
    nullptr, // Builtin_Array_Clear,
    nullptr, // Builtin_Array_Contains,
    nullptr, // Builtin_Array_IndexOf,
    nullptr, // Builtin_Array_Insert,
    nullptr, // Builtin_Array_PushBack,
    nullptr, // Builtin_Array_Remove,
    nullptr, // Builtin_Array_RemoveAt,

    &FillUserData_Builtin_TryGetComponentOfBaseType, // Builtin_TryGetComponentOfBaseType

    &FillUserData_Builtin_StartCoroutine, // Builtin_StartCoroutine,
    nullptr,                              // Builtin_StopCoroutine,
    nullptr,                              // Builtin_StopAllCoroutines,
    nullptr,                              // Builtin_WaitForAll,
    nullptr,                              // Builtin_WaitForAny,
    nullptr,                              // Builtin_Yield,

    nullptr, // LastBuiltin,
  };

  static_assert(PL_ARRAY_SIZE(s_TypeToFillUserDataFunctions) == plVisualScriptNodeDescription::Type::Count);

  plResult FillUserData(plVisualScriptCompiler::AstNode& inout_astNode, plVisualScriptCompiler* pCompiler, const plDocumentObject* pObject, const plDocumentObject* pEntryObject)
  {
    if (pObject == nullptr)
      return PL_SUCCESS;

    auto nodeType = inout_astNode.m_Type;
    PL_ASSERT_DEBUG(nodeType >= 0 && nodeType < PL_ARRAY_SIZE(s_TypeToFillUserDataFunctions), "Out of bounds access");
    auto func = s_TypeToFillUserDataFunctions[nodeType];

    if (func != nullptr)
    {
      PL_SUCCEED_OR_RETURN(func(inout_astNode, pCompiler, pObject, pEntryObject));
    }

    return PL_SUCCESS;
  }

} // namespace

//////////////////////////////////////////////////////////////////////////

plVisualScriptCompiler::CompiledModule::CompiledModule()
  : m_ConstantDataStorage(plSharedPtr<plVisualScriptDataDescription>(&m_ConstantDataDesc, nullptr))
{
  // Prevent the data desc from being deleted by fake shared ptr above
  m_ConstantDataDesc.AddRef();
}

plResult plVisualScriptCompiler::CompiledModule::Serialize(plStreamWriter& inout_stream) const
{
  PL_ASSERT_DEV(m_sScriptClassName.IsEmpty() == false, "Invalid script class name");

  plStringDeduplicationWriteContext stringDedup(inout_stream);

  plChunkStreamWriter chunk(stringDedup.Begin());
  chunk.BeginStream(1);

  {
    chunk.BeginChunk("Header", 1);
    chunk << m_sBaseClassName;
    chunk << m_sScriptClassName;
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("FunctionGraphs", 1);
    chunk << m_Functions.GetCount();

    for (auto& function : m_Functions)
    {
      chunk << function.m_sName;
      chunk << function.m_Type;
      chunk << function.m_CoroutineCreationMode;

      PL_SUCCEED_OR_RETURN(plVisualScriptGraphDescription::Serialize(function.m_NodeDescriptions, function.m_LocalDataDesc, chunk));
    }

    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("ConstantData", 1);
    PL_SUCCEED_OR_RETURN(m_ConstantDataDesc.Serialize(chunk));
    PL_SUCCEED_OR_RETURN(m_ConstantDataStorage.Serialize(chunk));
    chunk.EndChunk();
  }

  {
    chunk.BeginChunk("InstanceData", 1);
    PL_SUCCEED_OR_RETURN(m_InstanceDataDesc.Serialize(chunk));
    PL_SUCCEED_OR_RETURN(chunk.WriteHashTable(m_InstanceDataMapping.m_Content));
    chunk.EndChunk();
  }

  chunk.EndStream();

  return stringDedup.End();
}

//////////////////////////////////////////////////////////////////////////

// static
plUInt32 plVisualScriptCompiler::ConnectionHasher::Hash(const Connection& c)
{
  plUInt32 uiHashes[] = {
    plHashHelper<void*>::Hash(c.m_pPrev),
    plHashHelper<void*>::Hash(c.m_pCurrent),
    plHashHelper<plUInt32>::Hash(c.m_Type),
    plHashHelper<plUInt32>::Hash(c.m_uiPrevPinIndex),
  };
  return plHashingUtils::xxHash32(uiHashes, sizeof(uiHashes));
}

// static
bool plVisualScriptCompiler::ConnectionHasher::Equal(const Connection& a, const Connection& b)
{
  return a.m_pPrev == b.m_pPrev &&
         a.m_pCurrent == b.m_pCurrent &&
         a.m_Type == b.m_Type &&
         a.m_uiPrevPinIndex == b.m_uiPrevPinIndex;
}

//////////////////////////////////////////////////////////////////////////

plVisualScriptCompiler::plVisualScriptCompiler() = default;
plVisualScriptCompiler::~plVisualScriptCompiler() = default;

void plVisualScriptCompiler::InitModule(plStringView sBaseClassName, plStringView sScriptClassName)
{
  m_Module.m_sBaseClassName = sBaseClassName;
  m_Module.m_sScriptClassName = sScriptClassName;
}

plResult plVisualScriptCompiler::AddFunction(plStringView sName, const plDocumentObject* pEntryObject, const plDocumentObject* pParentObject)
{
  if (m_pManager == nullptr)
  {
    m_pManager = static_cast<const plVisualScriptNodeManager*>(pEntryObject->GetDocumentObjectManager());
  }
  PL_ASSERT_DEV(m_pManager == pEntryObject->GetDocumentObjectManager(), "Can't add functions from different document");

  for (auto& existingFunction : m_Module.m_Functions)
  {
    if (existingFunction.m_sName == sName)
    {
      plLog::Error("A function named '{}' already exists. Function names need to unique.", sName);
      return PL_FAILURE;
    }
  }

  AstNode* pEntryAstNode = BuildAST(pEntryObject);
  if (pEntryAstNode == nullptr)
    return PL_FAILURE;

  auto& function = m_Module.m_Functions.ExpandAndGetRef();
  function.m_sName = sName;
  function.m_Type = pEntryAstNode->m_Type;

  {
    auto pObjectWithCoroutineMode = pParentObject != nullptr ? pParentObject : pEntryObject;
    auto mode = pObjectWithCoroutineMode->GetTypeAccessor().GetValue("CoroutineMode");
    if (mode.IsA<plInt64>())
    {
      function.m_CoroutineCreationMode = static_cast<plScriptCoroutineCreationMode::Enum>(mode.Get<plInt64>());
    }
    else
    {
      function.m_CoroutineCreationMode = plScriptCoroutineCreationMode::AllowOverlap;
    }
  }

  m_EntryAstNodes.PushBack(pEntryAstNode);
  PL_ASSERT_DEBUG(m_Module.m_Functions.GetCount() == m_EntryAstNodes.GetCount(), "");

  return PL_SUCCESS;
}

plResult plVisualScriptCompiler::Compile(plStringView sDebugAstOutputPath)
{
  for (plUInt32 i = 0; i < m_Module.m_Functions.GetCount(); ++i)
  {
    auto& function = m_Module.m_Functions[i];
    AstNode* pEntryAstNode = m_EntryAstNodes[i];

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_00");

    PL_SUCCEED_OR_RETURN(ReplaceUnsupportedNodes(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_01_Replaced");

    PL_SUCCEED_OR_RETURN(InlineConstants(pEntryAstNode));
    PL_SUCCEED_OR_RETURN(InsertTypeConversions(pEntryAstNode));
    PL_SUCCEED_OR_RETURN(InlineVariables(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_02_TypeConv");

    PL_SUCCEED_OR_RETURN(BuildDataExecutions(pEntryAstNode));

    DumpAST(pEntryAstNode, sDebugAstOutputPath, function.m_sName, "_03_FlattenedExec");

    PL_SUCCEED_OR_RETURN(FillDataOutputConnections(pEntryAstNode));
    PL_SUCCEED_OR_RETURN(AssignLocalVariables(pEntryAstNode, function.m_LocalDataDesc));
    PL_SUCCEED_OR_RETURN(BuildNodeDescriptions(pEntryAstNode, function.m_NodeDescriptions));

    DumpGraph(function.m_NodeDescriptions, sDebugAstOutputPath, function.m_sName, "_Graph");

    m_PinIdToDataDesc.Clear();
  }

  PL_SUCCEED_OR_RETURN(FinalizeDataOffsets());
  PL_SUCCEED_OR_RETURN(FinalizeConstantData());

  return PL_SUCCESS;
}

plUInt32 plVisualScriptCompiler::GetPinId(const plVisualScriptPin* pPin)
{
  plUInt32 uiId = 0;
  if (pPin != nullptr && m_PinToId.TryGetValue(pPin, uiId))
    return uiId;

  uiId = m_uiNextPinId++;
  if (pPin != nullptr)
  {
    m_PinToId.Insert(pPin, uiId);
  }
  return uiId;
}

plVisualScriptCompiler::DataOutput& plVisualScriptCompiler::GetDataOutput(const DataInput& dataInput)
{
  if (dataInput.m_uiSourcePinIndex < dataInput.m_pSourceNode->m_Outputs.GetCount())
  {
    return dataInput.m_pSourceNode->m_Outputs[dataInput.m_uiSourcePinIndex];
  }

  PL_ASSERT_DEBUG(false, "This code should be never reached");
  static DataOutput dummy;
  return dummy;
}

plVisualScriptCompiler::AstNode& plVisualScriptCompiler::CreateAstNode(plVisualScriptNodeDescription::Type::Enum type, plVisualScriptDataType::Enum deductedDataType, bool bImplicitExecution)
{
  auto& node = m_AstNodes.ExpandAndGetRef();
  node.m_Type = type;
  node.m_DeductedDataType = deductedDataType;
  node.m_bImplicitExecution = bImplicitExecution;
  return node;
}

void plVisualScriptCompiler::AddDataInput(AstNode& node, AstNode* pSourceNode, plUInt8 uiSourcePinIndex, plVisualScriptDataType::Enum dataType)
{
  auto& dataInput = node.m_Inputs.ExpandAndGetRef();
  dataInput.m_pSourceNode = pSourceNode;
  dataInput.m_uiId = GetPinId(nullptr);
  dataInput.m_uiSourcePinIndex = uiSourcePinIndex;
  dataInput.m_DataType = dataType;
}

void plVisualScriptCompiler::AddDataOutput(AstNode& node, plVisualScriptDataType::Enum dataType)
{
  auto& dataOutput = node.m_Outputs.ExpandAndGetRef();
  dataOutput.m_uiId = GetPinId(nullptr);
  dataOutput.m_DataType = dataType;
}

plVisualScriptCompiler::DefaultInput plVisualScriptCompiler::GetDefaultPointerInput(const plRTTI* pDataType)
{
  DefaultInput defaultInput;
  if (m_DefaultInputs.TryGetValue(pDataType, defaultInput) == false)
  {
    if (pDataType == plGetStaticRTTI<plGameObject>() || pDataType == plGetStaticRTTI<plGameObjectHandle>())
    {
      auto& getOwnerNode = CreateAstNode(plVisualScriptNodeDescription::Type::GetScriptOwner, true);
      AddDataOutput(getOwnerNode, plVisualScriptDataType::TypedPointer);
      AddDataOutput(getOwnerNode, plVisualScriptDataType::GameObject);

      defaultInput.m_pSourceNode = &getOwnerNode;
      defaultInput.m_uiSourcePinIndex = 1;
      m_DefaultInputs.Insert(pDataType, defaultInput);
    }
    else if (pDataType == plGetStaticRTTI<plWorld>())
    {
      auto& getOwnerNode = CreateAstNode(plVisualScriptNodeDescription::Type::GetScriptOwner, true);
      AddDataOutput(getOwnerNode, plVisualScriptDataType::TypedPointer);

      defaultInput.m_pSourceNode = &getOwnerNode;
      defaultInput.m_uiSourcePinIndex = 0;
      m_DefaultInputs.Insert(pDataType, defaultInput);
    }
  }

  return defaultInput;
}

plVisualScriptCompiler::AstNode* plVisualScriptCompiler::CreateConstantNode(const plVariant& value)
{
  plVisualScriptDataType::Enum valueDataType = plVisualScriptDataType::FromVariantType(value.GetType());

  auto& constantNode = CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Constant, valueDataType, true);
  constantNode.m_Value = value;

  AddDataOutput(constantNode, valueDataType);

  return &constantNode;
}

plVisualScriptCompiler::AstNode* plVisualScriptCompiler::CreateJumpNode(AstNode* pTargetNode)
{
  auto& jumpNode = CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Jump);
  jumpNode.m_Value = plUInt64(*reinterpret_cast<size_t*>(&pTargetNode));

  return &jumpNode;
}

plVisualScriptCompiler::DataOffset plVisualScriptCompiler::GetInstanceDataOffset(plHashedString sName, plVisualScriptDataType::Enum dataType)
{
  plVisualScriptInstanceData instanceData;
  if (m_Module.m_InstanceDataMapping.m_Content.TryGetValue(sName, instanceData) == false)
  {
    PL_ASSERT_DEBUG(dataType < plVisualScriptDataType::Count, "Invalid data type");
    auto& offsetAndCount = m_Module.m_InstanceDataDesc.m_PerTypeInfo[dataType];
    instanceData.m_DataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
    instanceData.m_DataOffset.m_uiType = dataType;
    instanceData.m_DataOffset.m_uiSource = DataOffset::Source::Instance;
    ++offsetAndCount.m_uiCount;

    m_pManager->GetVariableDefaultValue(sName, instanceData.m_DefaultValue).AssertSuccess();

    m_Module.m_InstanceDataMapping.m_Content.Insert(sName, instanceData);
  }

  return instanceData.m_DataOffset;
}

plVisualScriptCompiler::AstNode* plVisualScriptCompiler::BuildAST(const plDocumentObject* pEntryObject)
{
  m_DefaultInputs.Clear();

  plHashTable<const plDocumentObject*, AstNode*> objectToAstNode;
  plHybridArray<const plVisualScriptPin*, 16> pins;

  auto CreateAstNodeFromObject = [&](const plDocumentObject* pObject) -> AstNode*
  {
    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    PL_ASSERT_DEV(pNodeDesc != nullptr, "Invalid node type");

    auto& astNode = CreateAstNode(pNodeDesc->m_Type, FinalizeDataType(GetDeductedType(pObject)), pNodeDesc->m_bImplicitExecution);
    if (FillUserData(astNode, this, pObject, pEntryObject).Failed())
      return nullptr;

    objectToAstNode.Insert(pObject, &astNode);

    return &astNode;
  };

  AstNode* pEntryAstNode = CreateAstNodeFromObject(pEntryObject);
  if (pEntryAstNode == nullptr)
    return nullptr;

  if (plVisualScriptNodeDescription::Type::IsEntry(pEntryAstNode->m_Type) == false)
  {
    auto& astNode = CreateAstNode(plVisualScriptNodeDescription::Type::EntryCall);
    astNode.m_Next.PushBack(pEntryAstNode);

    pEntryAstNode = &astNode;
  }

  plHybridArray<const plDocumentObject*, 64> nodeStack;
  nodeStack.PushBack(pEntryObject);

  while (nodeStack.IsEmpty() == false)
  {
    const plDocumentObject* pObject = nodeStack.PeekBack();
    nodeStack.PopBack();

    AstNode* pAstNode = nullptr;
    PL_VERIFY(objectToAstNode.TryGetValue(pObject, pAstNode), "Implementation error");

    if (plVisualScriptNodeDescription::Type::MakesOuterCoroutine(pAstNode->m_Type))
    {
      MarkAsCoroutine(pEntryAstNode);
    }

    m_pManager->GetInputDataPins(pObject, pins);
    plUInt32 uiNextInputPinIndex = 0;

    auto pNodeDesc = plVisualScriptNodeRegistry::GetSingleton()->GetNodeDescForType(pObject->GetType());
    for (auto& pinDesc : pNodeDesc->m_InputPins)
    {
      if (pinDesc.IsExecutionPin())
        continue;

      AstNode* pAstNodeToAddInput = pAstNode;
      bool bArrayInput = false;
      if (pinDesc.m_bReplaceWithArray && pinDesc.m_sDynamicPinProperty.IsEmpty() == false)
      {
        const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(pinDesc.m_sDynamicPinProperty);
        if (pProp == nullptr)
          return nullptr;

        if (pProp->GetCategory() == plPropertyCategory::Array)
        {
          auto pMakeArrayAstNode = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_MakeArray, true);
          AddDataOutput(*pMakeArrayAstNode, plVisualScriptDataType::Array);

          AddDataInput(*pAstNode, pMakeArrayAstNode, 0, plVisualScriptDataType::Array);

          pAstNodeToAddInput = pMakeArrayAstNode;
          bArrayInput = true;
        }
      }

      while (uiNextInputPinIndex < pins.GetCount())
      {
        auto pPin = pins[uiNextInputPinIndex];

        if (pPin->GetDynamicPinProperty() != pinDesc.m_sDynamicPinProperty)
          break;

        auto connections = m_pManager->GetConnections(*pPin);
        if (pPin->IsRequired() && connections.IsEmpty())
        {
          plLog::Error("Required input '{}' for '{}' is not connected", pPin->GetName(), GetNiceTypeName(pObject));
          return nullptr;
        }

        plVisualScriptDataType::Enum targetDataType = pPin->GetResolvedScriptDataType();
        if (targetDataType == plVisualScriptDataType::Invalid)
        {
          plLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pObject), pPin->GetName());
          return nullptr;
        }

        auto& dataInput = pAstNodeToAddInput->m_Inputs.ExpandAndGetRef();
        dataInput.m_uiId = GetPinId(pPin);
        dataInput.m_DataType = bArrayInput ? plVisualScriptDataType::Variant : FinalizeDataType(targetDataType);

        if (connections.IsEmpty())
        {
          if (plVisualScriptDataType::IsPointer(dataInput.m_DataType))
          {
            auto defaultInput = GetDefaultPointerInput(pPin->GetDataType());
            if (defaultInput.m_pSourceNode != nullptr)
            {
              dataInput.m_pSourceNode = defaultInput.m_pSourceNode;
              dataInput.m_uiSourcePinIndex = defaultInput.m_uiSourcePinIndex;
            }
          }
          else
          {
            plStringView sPropertyName = pPin->HasDynamicPinProperty() ? pPin->GetDynamicPinProperty() : pPin->GetName();

            plVariant value = pObject->GetTypeAccessor().GetValue(sPropertyName);
            if (value.IsValid() && pPin->HasDynamicPinProperty())
            {
              PL_ASSERT_DEBUG(value.IsA<plVariantArray>(), "Implementation error");
              value = value.Get<plVariantArray>()[pPin->GetElementIndex()];
            }

            if (value.IsA<plUuid>())
            {
              value = 0;
            }

            plVisualScriptDataType::Enum valueDataType = plVisualScriptDataType::FromVariantType(value.GetType());
            if (dataInput.m_DataType != plVisualScriptDataType::Variant)
            {
              value = value.ConvertTo(plVisualScriptDataType::GetVariantType(dataInput.m_DataType));
              if (value.IsValid() == false)
              {
                plLog::Error("Failed to convert '{}.{}' of type '{}' to '{}'.", GetNiceTypeName(pObject), pPin->GetName(), plVisualScriptDataType::GetName(valueDataType), plVisualScriptDataType::GetName(dataInput.m_DataType));
                return nullptr;
              }
            }

            dataInput.m_pSourceNode = CreateConstantNode(value);
            dataInput.m_uiSourcePinIndex = 0;
          }
        }
        else
        {
          auto& sourcePin = static_cast<const plVisualScriptPin&>(connections[0]->GetSourcePin());
          const plDocumentObject* pSourceObject = sourcePin.GetParent();

          AstNode* pSourceAstNode;
          if (objectToAstNode.TryGetValue(pSourceObject, pSourceAstNode) == false)
          {
            pSourceAstNode = CreateAstNodeFromObject(pSourceObject);
            if (pSourceAstNode == nullptr)
              return nullptr;

            nodeStack.PushBack(pSourceObject);
          }

          plVisualScriptDataType::Enum sourceDataType = sourcePin.GetResolvedScriptDataType();
          if (sourceDataType == plVisualScriptDataType::Invalid)
          {
            plLog::Error("Can't deduct type for pin '{}.{}'. The pin is not connected or all node properties are invalid.", GetNiceTypeName(pSourceObject), sourcePin.GetName());
            return nullptr;
          }

          if (sourcePin.CanConvertTo(*pPin) == false)
          {
            plLog::Error("Can't implicitly convert pin '{}.{}' of type '{}' connected to pin '{}.{}' of type '{}'", GetNiceTypeName(pSourceObject), sourcePin.GetName(), sourcePin.GetDataTypeName(), GetNiceTypeName(pObject), pPin->GetName(), pPin->GetDataTypeName());
            return nullptr;
          }

          dataInput.m_pSourceNode = pSourceAstNode;
          dataInput.m_uiSourcePinIndex = sourcePin.GetDataPinIndex();
        }

        ++uiNextInputPinIndex;
      }
    }

    m_pManager->GetOutputDataPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto& dataOutput = pAstNode->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId = GetPinId(pPin);
      dataOutput.m_DataType = FinalizeDataType(pPin->GetResolvedScriptDataType());
    }

    m_pManager->GetOutputExecutionPins(pObject, pins);
    for (auto pPin : pins)
    {
      auto connections = m_pManager->GetConnections(*pPin);
      if (connections.IsEmpty() || pPin->SplitExecution())
      {
        pAstNode->m_Next.PushBack(nullptr);
        continue;
      }

      PL_ASSERT_DEV(connections.GetCount() == 1, "Output execution pins should only have one connection");
      const plDocumentObject* pNextNode = connections[0]->GetTargetPin().GetParent();

      AstNode* pNextAstNode;
      if (objectToAstNode.TryGetValue(pNextNode, pNextAstNode) == false)
      {
        pNextAstNode = CreateAstNodeFromObject(pNextNode);
        if (pNextAstNode == nullptr)
          return nullptr;

        nodeStack.PushBack(pNextNode);
      }

      pAstNode->m_Next.PushBack(pNextAstNode);
    }
  }

  return pEntryAstNode;
}

void plVisualScriptCompiler::MarkAsCoroutine(AstNode* pEntryAstNode)
{
  switch (pEntryAstNode->m_Type)
  {
    case plVisualScriptNodeDescription::Type::EntryCall:
      pEntryAstNode->m_Type = plVisualScriptNodeDescription::Type::EntryCall_Coroutine;
      break;
    case plVisualScriptNodeDescription::Type::MessageHandler:
      pEntryAstNode->m_Type = plVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
      break;
    case plVisualScriptNodeDescription::Type::EntryCall_Coroutine:
    case plVisualScriptNodeDescription::Type::MessageHandler_Coroutine:
      // Already a coroutine
      break;
      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}

plResult plVisualScriptCompiler::ReplaceUnsupportedNodes(AstNode* pEntryAstNode)
{
  PL_SUCCEED_OR_RETURN(TraverseExecutionConnections(pEntryAstNode,
    [&](Connection& connection)
    {
      AstNode* pNode = connection.m_pCurrent;

      if (plVisualScriptNodeDescription::Type::IsLoop(pNode->m_Type))
      {
        if (ReplaceLoop(connection).Failed())
          return VisitorResult::Error;
      }

      return VisitorResult::Continue;
    }));

  return TraverseExecutionConnections(pEntryAstNode,
    [&](Connection& connection)
    {
      AstNode* pNode = connection.m_pCurrent;

      if (pNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_CompareExec)
      {
        const auto& dataInputA = pNode->m_Inputs[0];
        const auto& dataInputB = pNode->m_Inputs[1];

        auto& compareNode = CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Compare, pNode->m_DeductedDataType, false);
        compareNode.m_Value = pNode->m_Value;
        AddDataInput(compareNode, dataInputA.m_pSourceNode, dataInputA.m_uiSourcePinIndex, dataInputA.m_DataType);
        AddDataInput(compareNode, dataInputB.m_pSourceNode, dataInputB.m_uiSourcePinIndex, dataInputB.m_DataType);
        AddDataOutput(compareNode, plVisualScriptDataType::Bool);

        auto& branchNode = CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Branch);
        AddDataInput(branchNode, &compareNode, 0, plVisualScriptDataType::Bool);
        branchNode.m_Next.PushBack(pNode->m_Next[0]);
        branchNode.m_Next.PushBack(pNode->m_Next[1]);

        compareNode.m_Next.PushBack(&branchNode);
        connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = &compareNode;
        connection.m_pCurrent = &branchNode;
      }

      return VisitorResult::Continue;
    });
}

plResult plVisualScriptCompiler::ReplaceLoop(Connection& connection)
{
  AstNode* pLoopInitStart = nullptr;
  AstNode* pLoopInitEnd = nullptr;
  AstNode* pLoopConditionStart = nullptr;
  AstNode* pLoopConditionEnd = nullptr;
  AstNode* pLoopIncrementStart = nullptr;
  AstNode* pLoopIncrementEnd = nullptr;

  AstNode* pLoopElement = nullptr;
  AstNode* pLoopIndex = nullptr;

  AstNode* pLoopNode = connection.m_pCurrent;
  AstNode* pLoopBody = pLoopNode->m_Next[0];
  AstNode* pLoopCompleted = pLoopNode->m_Next[1];
  auto loopType = pLoopNode->m_Type;

  if (loopType == plVisualScriptNodeDescription::Type::Builtin_WhileLoop)
  {
    pLoopConditionEnd = pLoopNode->m_Inputs[0].m_pSourceNode;
    pLoopConditionEnd->m_bImplicitExecution = false;

    PL_SUCCEED_OR_RETURN(InlineConstants(pLoopConditionEnd));
    PL_SUCCEED_OR_RETURN(InsertTypeConversions(pLoopConditionEnd));
    PL_SUCCEED_OR_RETURN(InlineVariables(pLoopConditionEnd));

    plHybridArray<AstNode*, 64> nodeStack;
    PL_SUCCEED_OR_RETURN(BuildDataStack(pLoopConditionEnd, nodeStack));

    if (nodeStack.IsEmpty())
    {
      pLoopConditionStart = pLoopConditionEnd;
    }
    else
    {
      for (auto pDataNode : nodeStack)
      {
        pDataNode->m_bImplicitExecution = false;
      }

      pLoopConditionStart = nodeStack.PeekBack();

      AstNode* pLastDataNode = nodeStack[0];
      pLastDataNode->m_Next.PushBack(pLoopConditionEnd);
    }
  }
  else if (loopType == plVisualScriptNodeDescription::Type::Builtin_ForLoop)
  {
    auto& firstIndexInput = pLoopNode->m_Inputs[0];
    auto& lastIndexInput = pLoopNode->m_Inputs[1];

    // Loop Init
    {
      pLoopInitStart = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_ToInt, plVisualScriptDataType::Int);
      AddDataInput(*pLoopInitStart, firstIndexInput.m_pSourceNode, firstIndexInput.m_uiSourcePinIndex, firstIndexInput.m_DataType);
      AddDataOutput(*pLoopInitStart, plVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;

      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    {
      pLoopConditionStart = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Compare, plVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = plInt64(plComparisonOperator::LessEqual);
      AddDataInput(*pLoopConditionStart, pLoopInitStart, 0, plVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionStart, lastIndexInput.m_pSourceNode, lastIndexInput.m_uiSourcePinIndex, lastIndexInput.m_DataType);
      AddDataOutput(*pLoopConditionStart, plVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }

    // Loop Increment
    {
      pLoopIncrementStart = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Add, plVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, pLoopIndex, 0, plVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, CreateConstantNode(1), 0, plVisualScriptDataType::Int);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrementStart->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId = pLoopIndex->m_Outputs[0].m_uiId;
      dataOutput.m_DataType = plVisualScriptDataType::Int;

      pLoopIncrementEnd = pLoopIncrementStart;
    }
  }
  else if (loopType == plVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
           loopType == plVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
  {
    const bool isReverse = (loopType == plVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop);
    auto& arrayInput = pLoopNode->m_Inputs[0];

    // Loop Init
    if (isReverse)
    {
      pLoopInitStart = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      AddDataInput(*pLoopInitStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
      AddDataOutput(*pLoopInitStart, plVisualScriptDataType::Int);

      pLoopInitEnd = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Subtract, plVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, pLoopInitStart, 0, plVisualScriptDataType::Int);
      AddDataInput(*pLoopInitEnd, CreateConstantNode(1), 0, plVisualScriptDataType::Int);
      AddDataOutput(*pLoopInitEnd, plVisualScriptDataType::Int);

      pLoopInitStart->m_Next.PushBack(pLoopInitEnd);

      pLoopIndex = pLoopInitEnd;
    }
    else
    {
      pLoopInitStart = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_ToInt, plVisualScriptDataType::Int);
      AddDataInput(*pLoopInitStart, CreateConstantNode(0), 0, plVisualScriptDataType::Int);
      AddDataOutput(*pLoopInitStart, plVisualScriptDataType::Int);

      pLoopInitEnd = pLoopInitStart;

      pLoopIndex = pLoopInitStart;
    }

    // Loop Condition
    if (isReverse)
    {
      pLoopConditionStart = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Compare, plVisualScriptDataType::Int);
      pLoopConditionStart->m_Value = plInt64(plComparisonOperator::GreaterEqual);
      AddDataInput(*pLoopConditionStart, pLoopIndex, 0, plVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionStart, CreateConstantNode(0), 0, plVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionStart, plVisualScriptDataType::Bool);

      pLoopConditionEnd = pLoopConditionStart;
    }
    else
    {
      pLoopConditionStart = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Array_GetCount);
      AddDataInput(*pLoopConditionStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
      AddDataOutput(*pLoopConditionStart, plVisualScriptDataType::Int);

      pLoopConditionEnd = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Compare, plVisualScriptDataType::Int);
      pLoopConditionEnd->m_Value = plInt64(plComparisonOperator::Less);
      AddDataInput(*pLoopConditionEnd, pLoopIndex, 0, plVisualScriptDataType::Int);
      AddDataInput(*pLoopConditionEnd, pLoopConditionStart, 0, plVisualScriptDataType::Int);
      AddDataOutput(*pLoopConditionEnd, plVisualScriptDataType::Bool);

      pLoopConditionStart->m_Next.PushBack(pLoopConditionEnd);
    }

    // Loop Increment
    {
      auto incType = isReverse ? plVisualScriptNodeDescription::Type::Builtin_Subtract : plVisualScriptNodeDescription::Type::Builtin_Add;

      pLoopIncrementStart = &CreateAstNode(incType, plVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, pLoopIndex, 0, plVisualScriptDataType::Int);
      AddDataInput(*pLoopIncrementStart, CreateConstantNode(1), 0, plVisualScriptDataType::Int);

      // Dummy input that is not used at runtime but prevents the array from being re-used across the loop's lifetime
      AddDataInput(*pLoopIncrementStart, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);

      // Ensure to write to the same local variable by re-using the loop index output id.
      auto& dataOutput = pLoopIncrementStart->m_Outputs.ExpandAndGetRef();
      dataOutput.m_uiId = pLoopIndex->m_Outputs[0].m_uiId;
      dataOutput.m_DataType = plVisualScriptDataType::Int;

      pLoopIncrementEnd = pLoopIncrementStart;
    }

    pLoopElement = &CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Array_GetElement, plVisualScriptDataType::Invalid, true);
    AddDataInput(*pLoopElement, arrayInput.m_pSourceNode, arrayInput.m_uiSourcePinIndex, arrayInput.m_DataType);
    AddDataInput(*pLoopElement, pLoopIndex, 0, plVisualScriptDataType::Int);
    AddDataOutput(*pLoopElement, plVisualScriptDataType::Variant);
  }
  else
  {
    PL_ASSERT_NOT_IMPLEMENTED;
  }

  pLoopNode->m_Inputs.Clear();
  pLoopNode->m_Next.Clear();

  {
    auto& branchNode = CreateAstNode(plVisualScriptNodeDescription::Type::Builtin_Branch);
    AddDataInput(branchNode, pLoopConditionEnd, 0, plVisualScriptDataType::Bool);

    if (pLoopConditionStart->m_Type == plVisualScriptNodeDescription::Type::Builtin_Constant)
    {
      pLoopConditionStart = &branchNode;
    }
    else
    {
      pLoopConditionEnd->m_bImplicitExecution = false;
      pLoopConditionEnd->m_Next.PushBack(&branchNode);
    }

    branchNode.m_Next.PushBack(pLoopBody);      // True -> LoopBody
    branchNode.m_Next.PushBack(pLoopCompleted); // False -> Completed
    pLoopConditionEnd = &branchNode;
  }

  if (pLoopInitStart != nullptr)
  {
    connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pLoopInitStart;
    pLoopInitEnd->m_Next.PushBack(pLoopConditionStart);
  }
  else
  {
    connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pLoopConditionStart;
  }

  AstNode* pJumpNode = CreateJumpNode(pLoopConditionStart);
  if (pLoopIncrementStart != nullptr)
  {
    pLoopIncrementEnd->m_Next.PushBack(pJumpNode);
    pJumpNode = pLoopIncrementStart;
  }

  PL_SUCCEED_OR_RETURN(TraverseAllConnections(pLoopBody,
    [&](Connection& connection)
    {
      if (connection.m_pPrev == nullptr)
      {
        connection.m_pPrev = pLoopConditionEnd;
        connection.m_uiPrevPinIndex = 0;
      }

      if (plVisualScriptNodeDescription::Type::IsLoop(connection.m_pCurrent->m_Type))
      {
        if (ReplaceLoop(connection).Failed())
          return VisitorResult::Error;
      }

      if (connection.m_Type == ConnectionType::Data && connection.m_pCurrent->m_bImplicitExecution == false)
        return VisitorResult::Skip;

      AstNode* pNode = connection.m_pCurrent;

      if (pNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_Break)
      {
        connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pLoopCompleted;
        return VisitorResult::Continue;
      }

      for (auto& pNext : pNode->m_Next)
      {
        if (pNext == nullptr)
        {
          pNext = pJumpNode;
        }
      }

      for (auto& dataInput : pNode->m_Inputs)
      {
        if (loopType == plVisualScriptNodeDescription::Type::Builtin_ForEachLoop ||
            loopType == plVisualScriptNodeDescription::Type::Builtin_ReverseForEachLoop)
        {
          if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
          {
            dataInput.m_pSourceNode = pLoopElement;
          }
          else if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 1)
          {
            dataInput.m_pSourceNode = pLoopIndex;
            dataInput.m_uiSourcePinIndex = 0;
          }
        }
        else
        {
          if (dataInput.m_pSourceNode == pLoopNode && dataInput.m_uiSourcePinIndex == 0)
          {
            dataInput.m_pSourceNode = pLoopIndex;
          }
        }
      }

      return VisitorResult::Continue;
    }));

  connection.m_pPrev = pLoopConditionEnd;
  connection.m_pCurrent = pLoopCompleted;
  connection.m_uiPrevPinIndex = 1;

  return PL_SUCCESS;
}

plResult plVisualScriptCompiler::InsertTypeConversions(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      if (connection.m_Type == ConnectionType::Data)
      {
        auto& dataInput = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
        auto& dataOutput = GetDataOutput(dataInput);

        if (dataOutput.m_DataType != dataInput.m_DataType)
        {
          auto nodeType = plVisualScriptNodeDescription::Type::GetConversionType(dataInput.m_DataType);

          auto& astNode = CreateAstNode(nodeType, dataOutput.m_DataType, true);
          AddDataInput(astNode, dataInput.m_pSourceNode, dataInput.m_uiSourcePinIndex, dataOutput.m_DataType);
          AddDataOutput(astNode, dataInput.m_DataType);

          dataInput.m_pSourceNode = &astNode;
          dataInput.m_uiSourcePinIndex = 0;
        }
      }

      return VisitorResult::Continue;
    });
}

plResult plVisualScriptCompiler::InlineConstants(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      auto pCurrentNode = connection.m_pCurrent;
      for (auto& dataInput : pCurrentNode->m_Inputs)
      {
        if (m_PinIdToDataDesc.Contains(dataInput.m_uiId))
          continue;

        auto pSourceNode = dataInput.m_pSourceNode;
        if (pSourceNode == nullptr)
          continue;

        if (pSourceNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_Constant)
        {
          auto dataType = pSourceNode->m_DeductedDataType;

          plUInt32 uiIndex = plInvalidIndex;
          if (m_ConstantDataToIndex.TryGetValue(pSourceNode->m_Value, uiIndex) == false)
          {
            auto& offsetAndCount = m_Module.m_ConstantDataDesc.m_PerTypeInfo[dataType];
            uiIndex = offsetAndCount.m_uiCount;
            ++offsetAndCount.m_uiCount;

            m_ConstantDataToIndex.Insert(pSourceNode->m_Value, uiIndex);
          }

          DataDesc dataDesc;
          dataDesc.m_DataOffset = DataOffset(uiIndex, dataType, DataOffset::Source::Constant);
          m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);

          dataInput.m_pSourceNode = nullptr;
          dataInput.m_uiSourcePinIndex = 0;
        }
      }

      return VisitorResult::Continue;
    });
}

plResult plVisualScriptCompiler::InlineVariables(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      auto pCurrentNode = connection.m_pCurrent;
      for (auto& dataInput : pCurrentNode->m_Inputs)
      {
        if (m_PinIdToDataDesc.Contains(dataInput.m_uiId))
          continue;

        auto pSourceNode = dataInput.m_pSourceNode;
        if (pSourceNode == nullptr)
          continue;

        if (pSourceNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_GetVariable)
        {
          auto& dataOutput = pSourceNode->m_Outputs[0];

          plHashedString sName;
          sName.Assign(pSourceNode->m_Value.Get<plString>());

          DataDesc dataDesc;
          dataDesc.m_DataOffset = GetInstanceDataOffset(sName, dataOutput.m_DataType);
          m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);

          dataInput.m_pSourceNode = nullptr;
          dataInput.m_uiSourcePinIndex = 0;
        }
      }

      if (pCurrentNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_SetVariable ||
          pCurrentNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_IncVariable ||
          pCurrentNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_DecVariable)
      {
        plHashedString sName;
        sName.Assign(pCurrentNode->m_Value.Get<plString>());

        DataDesc dataDesc;
        dataDesc.m_DataOffset = GetInstanceDataOffset(sName, pCurrentNode->m_DeductedDataType);

        if (pCurrentNode->m_Type != plVisualScriptNodeDescription::Type::Builtin_SetVariable)
        {
          if (pCurrentNode->m_Inputs.IsEmpty())
          {
            AddDataInput(*pCurrentNode, nullptr, 0, pCurrentNode->m_DeductedDataType);
          }

          m_PinIdToDataDesc.Insert(pCurrentNode->m_Inputs[0].m_uiId, dataDesc);
        }

        {
          if (pCurrentNode->m_Outputs.IsEmpty())
          {
            AddDataOutput(*pCurrentNode, pCurrentNode->m_DeductedDataType);
          }

          m_PinIdToDataDesc.Insert(pCurrentNode->m_Outputs[0].m_uiId, dataDesc);
        }
      }

      return VisitorResult::Continue;
    });
}

plResult plVisualScriptCompiler::BuildDataStack(AstNode* pEntryAstNode, plDynamicArray<AstNode*>& out_Stack)
{
  plHashSet<const AstNode*> visitedNodes;
  out_Stack.Clear();

  PL_SUCCEED_OR_RETURN(TraverseDataConnections(
    pEntryAstNode,
    [&](const Connection& connection)
    {
      if (connection.m_pCurrent->m_bImplicitExecution == false)
        return VisitorResult::Skip;

      if (visitedNodes.Insert(connection.m_pCurrent))
      {
        // If the node was already visited, remove it again so it is moved to the top of the stack
        out_Stack.RemoveAndCopy(connection.m_pCurrent);
      }

      out_Stack.PushBack(connection.m_pCurrent);

      return VisitorResult::Continue;
    },
    false));

  // Make unique
  plHashTable<AstNode*, AstNode*> oldToNewNodes;
  for (plUInt32 i = out_Stack.GetCount(); i > 0; --i)
  {
    auto& pDataNode = out_Stack[i - 1];

    if (pDataNode->m_Next.IsEmpty())
    {
      // remap inputs to new nodes
      for (auto& dataInput : pDataNode->m_Inputs)
      {
        AstNode* pNewNode = nullptr;
        if (oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, pNewNode))
        {
          dataInput.m_pSourceNode = pNewNode;
        }
      }
    }
    else
    {
      auto& newDataNode = CreateAstNode(pDataNode->m_Type, pDataNode->m_DeductedDataType, pDataNode->m_bImplicitExecution);
      newDataNode.m_sTargetTypeName = pDataNode->m_sTargetTypeName;
      newDataNode.m_Value = pDataNode->m_Value;

      for (auto& dataInput : pDataNode->m_Inputs)
      {
        AstNode* pSourceNode = dataInput.m_pSourceNode;
        if (oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, pSourceNode) == false)
        {
          PL_ASSERT_DEBUG(dataInput.m_pSourceNode == nullptr || dataInput.m_pSourceNode->m_bImplicitExecution == false, "");
        }

        AddDataInput(newDataNode, pSourceNode, dataInput.m_uiSourcePinIndex, dataInput.m_DataType);

        DataDesc dataDesc;
        if (m_PinIdToDataDesc.TryGetValue(dataInput.m_uiId, dataDesc))
        {
          m_PinIdToDataDesc.Insert(newDataNode.m_Inputs.PeekBack().m_uiId, dataDesc);
        }
      }

      for (auto& dataOutput : pDataNode->m_Outputs)
      {
        AddDataOutput(newDataNode, dataOutput.m_DataType);

        DataDesc dataDesc;
        if (m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, dataDesc))
        {
          m_PinIdToDataDesc.Insert(newDataNode.m_Outputs.PeekBack().m_uiId, dataDesc);
        }
      }

      oldToNewNodes.Insert(pDataNode, &newDataNode);
      pDataNode = &newDataNode;
    }
  }

  // Connect next execution
  if (out_Stack.GetCount() > 1)
  {
    AstNode* pLastDataNode = out_Stack.PeekBack();
    for (plUInt32 i = out_Stack.GetCount() - 1; i > 0; --i)
    {
      auto& pDataNode = out_Stack[i - 1];
      pLastDataNode->m_Next.PushBack(pDataNode);
      pLastDataNode = pDataNode;
    }
  }

  // Remap inputs
  for (auto& dataInput : pEntryAstNode->m_Inputs)
  {
    if (dataInput.m_pSourceNode != nullptr)
    {
      oldToNewNodes.TryGetValue(dataInput.m_pSourceNode, dataInput.m_pSourceNode);
    }
  }

  return PL_SUCCESS;
}

plResult plVisualScriptCompiler::BuildDataExecutions(AstNode* pEntryAstNode)
{
  plHybridArray<Connection, 64> allExecConnections;

  PL_SUCCEED_OR_RETURN(TraverseExecutionConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      allExecConnections.PushBack(connection);
      return VisitorResult::Continue;
    }));

  plHybridArray<AstNode*, 64> nodeStack;
  plHashTable<AstNode*, AstNode*> nodeToFirstDataNode;

  for (const auto& connection : allExecConnections)
  {
    AstNode* pFirstDataNode = nullptr;
    if (nodeToFirstDataNode.TryGetValue(connection.m_pCurrent, pFirstDataNode) == false)
    {
      if (BuildDataStack(connection.m_pCurrent, nodeStack).Failed())
        return PL_FAILURE;

      if (nodeStack.IsEmpty() == false)
      {
        pFirstDataNode = nodeStack.PeekBack();

        AstNode* pLastDataNode = nodeStack[0];
        pLastDataNode->m_Next.PushBack(connection.m_pCurrent);
      }
    }

    if (pFirstDataNode != nullptr)
    {
      connection.m_pPrev->m_Next[connection.m_uiPrevPinIndex] = pFirstDataNode;
    }
    nodeToFirstDataNode.Insert(connection.m_pCurrent, pFirstDataNode);
  }

  return PL_SUCCESS;
}

plResult plVisualScriptCompiler::FillDataOutputConnections(AstNode* pEntryAstNode)
{
  return TraverseAllConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      if (connection.m_Type == ConnectionType::Data)
      {
        auto& dataInput = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
        auto& dataOutput = GetDataOutput(dataInput);

        PL_ASSERT_DEBUG(dataInput.m_pSourceNode == connection.m_pCurrent, "");
        if (dataOutput.m_TargetNodes.Contains(connection.m_pPrev) == false)
        {
          dataOutput.m_TargetNodes.PushBack(connection.m_pPrev);
        }
      }

      return VisitorResult::Continue;
    });
}

plResult plVisualScriptCompiler::AssignLocalVariables(AstNode* pEntryAstNode, plVisualScriptDataDescription& inout_localDataDesc)
{
  plDynamicArray<DataOffset> freeDataOffsets;

  return TraverseExecutionConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      // Outputs first so we don't end up using the same data as input and output
      for (auto& dataOutput : connection.m_pCurrent->m_Outputs)
      {
        if (m_PinIdToDataDesc.Contains(dataOutput.m_uiId))
          continue;

        if (dataOutput.m_TargetNodes.IsEmpty() == false)
        {
          DataOffset dataOffset;
          dataOffset.m_uiType = dataOutput.m_DataType;

          for (plUInt32 i = 0; i < freeDataOffsets.GetCount(); ++i)
          {
            auto freeDataOffset = freeDataOffsets[i];
            if (freeDataOffset.m_uiType == dataOffset.m_uiType)
            {
              dataOffset = freeDataOffset;
              freeDataOffsets.RemoveAtAndSwap(i);
              break;
            }
          }

          if (dataOffset.IsValid() == false)
          {
            PL_ASSERT_DEBUG(dataOffset.GetType() < plVisualScriptDataType::Count, "Invalid data type");
            auto& offsetAndCount = inout_localDataDesc.m_PerTypeInfo[dataOffset.m_uiType];
            dataOffset.m_uiByteOffset = offsetAndCount.m_uiCount;
            ++offsetAndCount.m_uiCount;
          }

          DataDesc dataDesc;
          dataDesc.m_DataOffset = dataOffset;
          dataDesc.m_uiUsageCounter = dataOutput.m_TargetNodes.GetCount();
          m_PinIdToDataDesc.Insert(dataOutput.m_uiId, dataDesc);
        }
      }

      for (auto& dataInput : connection.m_pCurrent->m_Inputs)
      {
        if (m_PinIdToDataDesc.Contains(dataInput.m_uiId) || dataInput.m_pSourceNode == nullptr)
          continue;

        auto& dataOutput = GetDataOutput(dataInput);
        DataDesc* pDataDesc = nullptr;
        m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, pDataDesc);
        if (pDataDesc == nullptr)
        {
          plLog::Error("Internal Compiler Error: Local variable for output id {} is not yet assigned.", dataOutput.m_uiId);
          return VisitorResult::Error;
        }

        --pDataDesc->m_uiUsageCounter;
        if (pDataDesc->m_uiUsageCounter == 0 && pDataDesc->m_DataOffset.IsLocal())
        {
          freeDataOffsets.PushBack(pDataDesc->m_DataOffset);
        }

        // Make a copy first because Insert() might re-allocate and the pointer might point to dead memory afterwards.
        DataDesc dataDesc = *pDataDesc;
        m_PinIdToDataDesc.Insert(dataInput.m_uiId, dataDesc);
      }

      return VisitorResult::Continue;
    });
}

plResult plVisualScriptCompiler::BuildNodeDescriptions(AstNode* pEntryAstNode, plDynamicArray<plVisualScriptNodeDescription>& out_NodeDescriptions)
{
  plHashTable<const AstNode*, plUInt32> astNodeToNodeDescIndices;
  out_NodeDescriptions.Clear();

  auto CreateNodeDesc = [&](const AstNode& astNode, plUInt32& out_uiNodeDescIndex) -> plResult
  {
    out_uiNodeDescIndex = out_NodeDescriptions.GetCount();

    auto& nodeDesc = out_NodeDescriptions.ExpandAndGetRef();
    nodeDesc.m_Type = astNode.m_Type;
    nodeDesc.m_DeductedDataType = astNode.m_DeductedDataType;
    nodeDesc.m_sTargetTypeName = astNode.m_sTargetTypeName;
    nodeDesc.m_Value = astNode.m_Value;

    for (auto& dataInput : astNode.m_Inputs)
    {
      DataDesc dataDesc;
      m_PinIdToDataDesc.TryGetValue(dataInput.m_uiId, dataDesc);
      nodeDesc.m_InputDataOffsets.PushBack(dataDesc.m_DataOffset);
    }

    for (auto& dataOutput : astNode.m_Outputs)
    {
      DataDesc dataDesc;
      m_PinIdToDataDesc.TryGetValue(dataOutput.m_uiId, dataDesc);
      nodeDesc.m_OutputDataOffsets.PushBack(dataDesc.m_DataOffset);
    }

    astNodeToNodeDescIndices.Insert(&astNode, out_uiNodeDescIndex);
    return PL_SUCCESS;
  };

  plUInt32 uiNodeDescIndex = 0;
  PL_SUCCEED_OR_RETURN(CreateNodeDesc(*pEntryAstNode, uiNodeDescIndex));

  return TraverseExecutionConnections(pEntryAstNode,
    [&](const Connection& connection)
    {
      plUInt32 uiCurrentIndex = 0;
      if (astNodeToNodeDescIndices.TryGetValue(connection.m_pCurrent, uiCurrentIndex) == false)
      {
        return VisitorResult::Skip;
      }

      auto pNodeDesc = &out_NodeDescriptions[uiCurrentIndex];
      if (pNodeDesc->m_ExecutionIndices.GetCount() == connection.m_pCurrent->m_Next.GetCount())
      {
        return VisitorResult::Continue;
      }

      for (auto pNextAstNode : connection.m_pCurrent->m_Next)
      {
        if (pNextAstNode == nullptr)
        {
          pNodeDesc->m_ExecutionIndices.PushBack(static_cast<plUInt16>(plInvalidIndex));
        }
        else if (pNextAstNode->m_Type == plVisualScriptNodeDescription::Type::Builtin_Jump)
        {
          plUInt64 uiPtr = pNextAstNode->m_Value.Get<plUInt64>();
          AstNode* pTargetAstNode = *reinterpret_cast<AstNode**>(&uiPtr);

          plUInt32 uiNextIndex = 0;
          if (astNodeToNodeDescIndices.TryGetValue(pTargetAstNode, uiNextIndex) == false)
            return VisitorResult::Error;

          pNodeDesc->m_ExecutionIndices.PushBack(uiNextIndex);
        }
        else
        {
          plUInt32 uiNextIndex = 0;
          if (astNodeToNodeDescIndices.TryGetValue(pNextAstNode, uiNextIndex) == false)
          {
            if (CreateNodeDesc(*pNextAstNode, uiNextIndex).Failed())
              return VisitorResult::Error;

            // array might have been resized, fetch node desc again
            pNodeDesc = &out_NodeDescriptions[uiCurrentIndex];
          }

          pNodeDesc->m_ExecutionIndices.PushBack(uiNextIndex);
        }
      }

      return VisitorResult::Continue;
    });
}

plResult plVisualScriptCompiler::TraverseExecutionConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/)
{
  m_ReportedConnections.Clear();
  plHybridArray<AstNode*, 64> nodeStack;

  {
    Connection connection = {nullptr, pEntryAstNode, ConnectionType::Execution, plInvalidIndex};
    auto res = func(connection);
    if (res == VisitorResult::Skip || res == VisitorResult::Stop)
      return PL_SUCCESS;
    if (res == VisitorResult::Error)
      return PL_FAILURE;

    if (connection.m_pCurrent != nullptr)
    {
      nodeStack.PushBack(connection.m_pCurrent);
    }
  }

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pCurrentAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    for (plUInt32 i = 0; i < pCurrentAstNode->m_Next.GetCount(); ++i)
    {
      auto pNextAstNode = pCurrentAstNode->m_Next[i];
      PL_ASSERT_DEBUG(pNextAstNode != pCurrentAstNode, "");

      if (pNextAstNode == nullptr)
        continue;

      Connection connection = {pCurrentAstNode, pNextAstNode, ConnectionType::Execution, i};
      if (bDeduplicate && m_ReportedConnections.Insert(connection))
        continue;

      auto res = func(connection);
      if (res == VisitorResult::Skip)
        continue;
      if (res == VisitorResult::Stop)
        return PL_SUCCESS;
      if (res == VisitorResult::Error)
        return PL_FAILURE;

      if (connection.m_pCurrent != nullptr)
      {
        nodeStack.PushBack(connection.m_pCurrent);
      }
    }
  }

  return PL_SUCCESS;
}

plResult plVisualScriptCompiler::TraverseDataConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/, bool bClearReportedConnections /*= true*/)
{
  if (bClearReportedConnections)
  {
    m_ReportedConnections.Clear();
  }

  plHybridArray<AstNode*, 64> nodeStack;

  nodeStack.PushBack(pEntryAstNode);

  while (nodeStack.IsEmpty() == false)
  {
    AstNode* pCurrentAstNode = nodeStack.PeekBack();
    nodeStack.PopBack();

    for (plUInt32 i = 0; i < pCurrentAstNode->m_Inputs.GetCount(); ++i)
    {
      auto& dataInput = pCurrentAstNode->m_Inputs[i];

      if (dataInput.m_pSourceNode == nullptr)
        continue;

      Connection connection = {pCurrentAstNode, dataInput.m_pSourceNode, ConnectionType::Data, i};
      if (bDeduplicate && m_ReportedConnections.Insert(connection))
        continue;

      auto res = func(connection);
      if (res == VisitorResult::Skip)
        continue;
      if (res == VisitorResult::Stop)
        return PL_SUCCESS;
      if (res == VisitorResult::Error)
        return PL_FAILURE;

      if (connection.m_pCurrent != nullptr)
      {
        nodeStack.PushBack(connection.m_pCurrent);
      }
    }
  }

  return PL_SUCCESS;
}

plResult plVisualScriptCompiler::TraverseAllConnections(AstNode* pEntryAstNode, AstNodeVisitorFunc func, bool bDeduplicate /*= true*/)
{
  return TraverseExecutionConnections(
    pEntryAstNode,
    [&](Connection& connection)
    {
      auto res = func(connection);
      if (res != VisitorResult::Continue)
        return res;

      if (TraverseDataConnections(connection.m_pCurrent, func, bDeduplicate, false).Failed())
        return VisitorResult::Error;

      return VisitorResult::Continue;
    },
    bDeduplicate);
}

plResult plVisualScriptCompiler::FinalizeDataOffsets()
{
  m_Module.m_InstanceDataDesc.CalculatePerTypeStartOffsets();
  m_Module.m_ConstantDataDesc.CalculatePerTypeStartOffsets();

  auto GetDataDesc = [this](const CompiledFunction& function, DataOffset dataOffset) -> const plVisualScriptDataDescription*
  {
    switch (dataOffset.GetSource())
    {
      case DataOffset::Source::Local:
        return &function.m_LocalDataDesc;
      case DataOffset::Source::Instance:
        return &m_Module.m_InstanceDataDesc;
      case DataOffset::Source::Constant:
        return &m_Module.m_ConstantDataDesc;
        PL_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return nullptr;
  };

  for (auto& function : m_Module.m_Functions)
  {
    function.m_LocalDataDesc.CalculatePerTypeStartOffsets();

    for (auto& nodeDesc : function.m_NodeDescriptions)
    {
      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        dataOffset = GetDataDesc(function, dataOffset)->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        PL_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Cannot write to constant data");
        dataOffset = GetDataDesc(function, dataOffset)->GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
      }
    }
  }

  for (auto& it : m_Module.m_InstanceDataMapping.m_Content)
  {
    auto& dataOffset = it.Value().m_DataOffset;
    dataOffset = m_Module.m_InstanceDataDesc.GetOffset(dataOffset.GetType(), dataOffset.m_uiByteOffset, dataOffset.GetSource());
  }

  return PL_SUCCESS;
}

plResult plVisualScriptCompiler::FinalizeConstantData()
{
  m_Module.m_ConstantDataStorage.AllocateStorage();

  for (auto& it : m_ConstantDataToIndex)
  {
    const plVariant& value = it.Key();
    plUInt32 uiIndex = it.Value();

    auto scriptDataType = plVisualScriptDataType::FromVariantType(value.GetType());
    if (scriptDataType == plVisualScriptDataType::Invalid)
    {
      scriptDataType = plVisualScriptDataType::Variant;
    }

    auto dataOffset = m_Module.m_ConstantDataDesc.GetOffset(scriptDataType, uiIndex, DataOffset::Source::Constant);

    m_Module.m_ConstantDataStorage.SetDataFromVariant(dataOffset, value, 0);
  }

  return PL_SUCCESS;
}

void plVisualScriptCompiler::DumpAST(AstNode* pEntryAstNode, plStringView sOutputPath, plStringView sFunctionName, plStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  plDGMLGraph dgmlGraph;
  {
    plHashTable<const AstNode*, plUInt32> nodeCache;
    plHashTable<plUInt64, plString> connectionCache;
    plStringBuilder sb;

    TraverseAllConnections(pEntryAstNode,
      [&](const Connection& connection)
      {
        AstNode* pAstNode = connection.m_pCurrent;

        plUInt32 uiGraphNode = 0;
        if (nodeCache.TryGetValue(pAstNode, uiGraphNode) == false)
        {
          const char* szTypeName = plVisualScriptNodeDescription::Type::GetName(pAstNode->m_Type);
          sb = szTypeName;
          if (pAstNode->m_sTargetTypeName.IsEmpty() == false)
          {
            sb.Append("\n", pAstNode->m_sTargetTypeName);
          }
          if (pAstNode->m_DeductedDataType != plVisualScriptDataType::Invalid)
          {
            sb.Append("\nDataType: ", plVisualScriptDataType::GetName(pAstNode->m_DeductedDataType));
          }
          sb.AppendFormat("\nImplicitExec: {}", pAstNode->m_bImplicitExecution);
          if (pAstNode->m_Value.IsValid())
          {
            sb.AppendFormat("\nValue: {}", pAstNode->m_Value);
          }

          float colorX = plSimdRandom::FloatZeroToOne(plSimdVec4i(plHashingUtils::StringHash(szTypeName))).x();

          plDGMLGraph::NodeDesc nd;
          nd.m_Color = plColorScheme::LightUI(colorX);
          uiGraphNode = dgmlGraph.AddNode(sb, &nd);
          nodeCache.Insert(pAstNode, uiGraphNode);
        }

        if (connection.m_pPrev != nullptr)
        {
          plUInt32 uiPrevGraphNode = 0;
          PL_VERIFY(nodeCache.TryGetValue(connection.m_pPrev, uiPrevGraphNode), "");

          if (connection.m_Type == ConnectionType::Execution)
          {
            plUInt64 uiConnectionKey = uiPrevGraphNode | plUInt64(uiGraphNode) << 32;
            plString& sLabel = connectionCache[uiConnectionKey];

            plStringBuilder sb = sLabel;
            if (sb.IsEmpty() == false)
            {
              sb.Append(" + ");
            }
            sb.Append("Exec");
            sLabel = sb;
          }
          else
          {
            plUInt64 uiConnectionKey = uiGraphNode | plUInt64(uiPrevGraphNode) << 32;
            plString& sLabel = connectionCache[uiConnectionKey];

            auto& dataInput = connection.m_pPrev->m_Inputs[connection.m_uiPrevPinIndex];
            auto& dataOutput = GetDataOutput(dataInput);

            plStringBuilder sb = sLabel;
            if (sb.IsEmpty() == false)
            {
              sb.Append(" + ");
            }
            sb.AppendFormat("o{}:{} (id: {})->i{}:{} (id: {})", dataInput.m_uiSourcePinIndex, plVisualScriptDataType::GetName(dataOutput.m_DataType), dataOutput.m_uiId, connection.m_uiPrevPinIndex, plVisualScriptDataType::GetName(dataInput.m_DataType), dataInput.m_uiId);
            sLabel = sb;
          }
        }

        return VisitorResult::Continue;
      })
      .IgnoreResult();

    for (auto& it : connectionCache)
    {
      plUInt32 uiSource = it.Key() & 0xFFFFFFFF;
      plUInt32 uiTarget = it.Key() >> 32;

      dgmlGraph.AddConnection(uiSource, uiTarget, it.Value());
    }
  }

  plStringView sExt = sOutputPath.GetFileExtension();
  plStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  plDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    plLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    plLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}

void plVisualScriptCompiler::DumpGraph(plArrayPtr<const plVisualScriptNodeDescription> nodeDescriptions, plStringView sOutputPath, plStringView sFunctionName, plStringView sSuffix)
{
  if (sOutputPath.IsEmpty())
    return;

  plDGMLGraph dgmlGraph;
  {
    plStringBuilder sTmp;
    for (auto& nodeDesc : nodeDescriptions)
    {
      plStringView sTypeName = plVisualScriptNodeDescription::Type::GetName(nodeDesc.m_Type);
      sTmp = sTypeName;

      nodeDesc.AppendUserDataName(sTmp);

      for (auto& dataOffset : nodeDesc.m_InputDataOffsets)
      {
        sTmp.AppendFormat("\n Input {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), plVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);

        if (dataOffset.GetSource() == DataOffset::Source::Constant)
        {
          for (auto& it : m_ConstantDataToIndex)
          {
            auto scriptDataType = plVisualScriptDataType::FromVariantType(it.Key().GetType());
            if (scriptDataType == dataOffset.GetType() && it.Value() == dataOffset.m_uiByteOffset)
            {
              sTmp.AppendFormat(" ({})", it.Key());
              break;
            }
          }
        }
      }

      for (auto& dataOffset : nodeDesc.m_OutputDataOffsets)
      {
        sTmp.AppendFormat("\n Output {} {}[{}]", DataOffset::Source::GetName(dataOffset.GetSource()), plVisualScriptDataType::GetName(dataOffset.GetType()), dataOffset.m_uiByteOffset);
      }

      float colorX = plSimdRandom::FloatZeroToOne(plSimdVec4i(plHashingUtils::StringHash(sTypeName))).x();

      plDGMLGraph::NodeDesc nd;
      nd.m_Color = plColorScheme::LightUI(colorX);

      dgmlGraph.AddNode(sTmp, &nd);
    }

    for (plUInt32 i = 0; i < nodeDescriptions.GetCount(); ++i)
    {
      for (auto uiNextIndex : nodeDescriptions[i].m_ExecutionIndices)
      {
        if (uiNextIndex == plSmallInvalidIndex)
          continue;

        dgmlGraph.AddConnection(i, uiNextIndex);
      }
    }
  }

  plStringView sExt = sOutputPath.GetFileExtension();
  plStringBuilder sFullPath;
  sFullPath.Append(sOutputPath.GetFileDirectory(), sOutputPath.GetFileName(), "_", sFunctionName, sSuffix);
  sFullPath.Append(".", sExt);

  plDGMLGraphWriter dgmlGraphWriter;
  if (dgmlGraphWriter.WriteGraphToFile(sFullPath, dgmlGraph).Succeeded())
  {
    plLog::Info("AST was dumped to: {}", sFullPath);
  }
  else
  {
    plLog::Error("Failed to dump AST to: {}", sFullPath);
  }
}
