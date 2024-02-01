#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <VisualScriptPlugin/Runtime/VisualScriptInstance.h>
#include <VisualScriptPlugin/Runtime/VisualScriptNodeUserData.h>

plVisualScriptGraphDescription::ExecuteFunction GetExecuteFunction(plVisualScriptNodeDescription::Type::Enum nodeType, plVisualScriptDataType::Enum dataType);

namespace
{
  static const char* s_NodeDescTypeNames[] = {
    "", // Invalid,
    "EntryCall",
    "EntryCall_Coroutine",
    "MessageHandler",
    "MessageHandler_Coroutine",
    "ReflectedFunction",
    "GetReflectedProperty",
    "SetReflectedProperty",
    "InplaceCoroutine",
    "GetScriptOwner",
    "SendMessage",

    "", // FirstBuiltin,

    "Builtin_Constant",
    "Builtin_GetVariable",
    "Builtin_SetVariable",
    "Builtin_IncVariable",
    "Builtin_DecVariable",

    "Builtin_Branch",
    "Builtin_Switch",
    "Builtin_WhileLoop",
    "Builtin_ForLoop",
    "Builtin_ForEachLoop",
    "Builtin_ReverseForEachLoop",
    "Builtin_Break",
    "Builtin_Jump",

    "Builtin_And",
    "Builtin_Or",
    "Builtin_Not",
    "Builtin_Compare",
    "Builtin_CompareExec",
    "Builtin_IsValid",
    "Builtin_Select",

    "Builtin_Add",
    "Builtin_Subtract",
    "Builtin_Multiply",
    "Builtin_Divide",
    "Builtin_Expression",

    "Builtin_ToBool",
    "Builtin_ToByte",
    "Builtin_ToInt",
    "Builtin_ToInt64",
    "Builtin_ToFloat",
    "Builtin_ToDouble",
    "Builtin_ToString",
    "Builtin_String_Format",
    "Builtin_ToHashedString",
    "Builtin_ToVariant",
    "Builtin_Variant_ConvertTo",

    "Builtin_MakeArray",
    "Builtin_Array_GetElement",
    "Builtin_Array_SetElement",
    "Builtin_Array_GetCount",
    "Builtin_Array_IsEmpty",
    "Builtin_Array_Clear",
    "Builtin_Array_Contains",
    "Builtin_Array_IndexOf",
    "Builtin_Array_Insert",
    "Builtin_Array_PushBack",
    "Builtin_Array_Remove",
    "Builtin_Array_RemoveAt",

    "Builtin_TryGetComponentOfBaseType",

    "Builtin_StartCoroutine",
    "Builtin_StopCoroutine",
    "Builtin_StopAllCoroutines",
    "Builtin_WaitForAll",
    "Builtin_WaitForAny",
    "Builtin_Yield",

    "", // LastBuiltin,
  };
  static_assert(PL_ARRAY_SIZE(s_NodeDescTypeNames) == (size_t)plVisualScriptNodeDescription::Type::Count);

  template <typename T>
  plResult WriteNodeArray(plArrayPtr<T> a, plStreamWriter& inout_stream)
  {
    plUInt16 uiCount = static_cast<plUInt16>(a.GetCount());
    inout_stream << uiCount;

    return inout_stream.WriteBytes(a.GetPtr(), a.GetCount() * sizeof(T));
  }

} // namespace

// static
plVisualScriptNodeDescription::Type::Enum plVisualScriptNodeDescription::Type::GetConversionType(plVisualScriptDataType::Enum targetDataType)
{
  static_assert(Builtin_ToBool + (plVisualScriptDataType::Bool - plVisualScriptDataType::Bool) == Builtin_ToBool);
  static_assert(Builtin_ToBool + (plVisualScriptDataType::Byte - plVisualScriptDataType::Bool) == Builtin_ToByte);
  static_assert(Builtin_ToBool + (plVisualScriptDataType::Int - plVisualScriptDataType::Bool) == Builtin_ToInt);
  static_assert(Builtin_ToBool + (plVisualScriptDataType::Int64 - plVisualScriptDataType::Bool) == Builtin_ToInt64);
  static_assert(Builtin_ToBool + (plVisualScriptDataType::Float - plVisualScriptDataType::Bool) == Builtin_ToFloat);
  static_assert(Builtin_ToBool + (plVisualScriptDataType::Double - plVisualScriptDataType::Bool) == Builtin_ToDouble);

  if (plVisualScriptDataType::IsNumber(targetDataType))
    return static_cast<Enum>(Builtin_ToBool + (targetDataType - plVisualScriptDataType::Bool));

  if (targetDataType == plVisualScriptDataType::String)
    return Builtin_ToString;

  if (targetDataType == plVisualScriptDataType::HashedString)
    return Builtin_ToHashedString;

  if (targetDataType == plVisualScriptDataType::Variant)
    return Builtin_ToVariant;

  PL_ASSERT_NOT_IMPLEMENTED;
  return Invalid;
}

// static
const char* plVisualScriptNodeDescription::Type::GetName(Enum type)
{
  PL_ASSERT_DEBUG(type >= 0 && type < PL_ARRAY_SIZE(s_NodeDescTypeNames), "Out of bounds access");
  return s_NodeDescTypeNames[type];
}

void plVisualScriptNodeDescription::AppendUserDataName(plStringBuilder& out_sResult) const
{
  if (auto func = GetUserDataContext(m_Type).m_ToStringFunc)
  {
    out_sResult.Append(" ");

    func(*this, out_sResult);
  }
}

//////////////////////////////////////////////////////////////////////////

plVisualScriptGraphDescription::plVisualScriptGraphDescription()
{
  static_assert(sizeof(Node) == 64);
}

plVisualScriptGraphDescription::~plVisualScriptGraphDescription() = default;

static const plTypeVersion s_uiVisualScriptGraphDescriptionVersion = 3;

// static
plResult plVisualScriptGraphDescription::Serialize(plArrayPtr<const plVisualScriptNodeDescription> nodes, const plVisualScriptDataDescription& localDataDesc, plStreamWriter& inout_stream)
{
  inout_stream.WriteVersion(s_uiVisualScriptGraphDescriptionVersion);

  plDefaultMemoryStreamStorage streamStorage;
  plMemoryStreamWriter stream(&streamStorage);
  plUInt32 additionalDataSize = 0;
  {
    for (auto& nodeDesc : nodes)
    {
      stream << nodeDesc.m_Type;
      stream << nodeDesc.m_DeductedDataType;
      PL_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_ExecutionIndices.GetArrayPtr(), stream));
      PL_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_InputDataOffsets.GetArrayPtr(), stream));
      PL_SUCCEED_OR_RETURN(WriteNodeArray(nodeDesc.m_OutputDataOffsets.GetArrayPtr(), stream));

      ExecutionIndicesArray::AddAdditionalDataSize(nodeDesc.m_ExecutionIndices, additionalDataSize);
      InputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_InputDataOffsets, additionalDataSize);
      OutputDataOffsetsArray::AddAdditionalDataSize(nodeDesc.m_OutputDataOffsets, additionalDataSize);

      if (auto func = GetUserDataContext(nodeDesc.m_Type).m_SerializeFunc)
      {
        plUInt32 uiSize = 0;
        plUInt32 uiAlignment = 0;
        PL_SUCCEED_OR_RETURN(func(nodeDesc, stream, uiSize, uiAlignment));

        UserDataArray::AddAdditionalDataSize(uiSize, uiAlignment, additionalDataSize);
      }
    }
  }

  const plUInt32 uiRequiredStorageSize = nodes.GetCount() * sizeof(Node) + additionalDataSize;
  inout_stream << uiRequiredStorageSize;
  inout_stream << nodes.GetCount();

  PL_SUCCEED_OR_RETURN(streamStorage.CopyToStream(inout_stream));

  PL_SUCCEED_OR_RETURN(localDataDesc.Serialize(inout_stream));

  return PL_SUCCESS;
}

plResult plVisualScriptGraphDescription::Deserialize(plStreamReader& inout_stream)
{
  plTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptGraphDescriptionVersion);
  if (uiVersion < 3)
  {
    plLog::Error("Invalid visual script desc version. Expected >= 3 but got {}. Visual Script needs re-export", uiVersion);
    return PL_FAILURE;
  }

  {
    plUInt32 uiStorageSize;
    inout_stream >> uiStorageSize;

    m_Storage.SetCountUninitialized(uiStorageSize);
    m_Storage.ZeroFill();
  }

  plUInt32 uiNumNodes;
  inout_stream >> uiNumNodes;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();
  auto nodes = plMakeArrayPtr(reinterpret_cast<Node*>(pData), uiNumNodes);

  plUInt8* pAdditionalData = pData + uiNumNodes * sizeof(Node);

  for (auto& node : nodes)
  {
    inout_stream >> node.m_Type;
    inout_stream >> node.m_DeductedDataType;

    node.m_Function = GetExecuteFunction(node.m_Type, node.m_DeductedDataType);

    PL_SUCCEED_OR_RETURN(node.m_ExecutionIndices.ReadFromStream(node.m_NumExecutionIndices, inout_stream, pAdditionalData));
    PL_SUCCEED_OR_RETURN(node.m_InputDataOffsets.ReadFromStream(node.m_NumInputDataOffsets, inout_stream, pAdditionalData));
    PL_SUCCEED_OR_RETURN(node.m_OutputDataOffsets.ReadFromStream(node.m_NumOutputDataOffsets, inout_stream, pAdditionalData));

    if (auto func = GetUserDataContext(node.m_Type).m_DeserializeFunc)
    {
      PL_SUCCEED_OR_RETURN(func(node, inout_stream, pAdditionalData));
    }
  }

  m_Nodes = nodes;

  plSharedPtr<plVisualScriptDataDescription> pLocalDataDesc = PL_SCRIPT_NEW(plVisualScriptDataDescription);
  PL_SUCCEED_OR_RETURN(pLocalDataDesc->Deserialize(inout_stream));
  m_pLocalDataDesc = pLocalDataDesc;

  return PL_SUCCESS;
}

plScriptMessageDesc plVisualScriptGraphDescription::GetMessageDesc() const
{
  auto pEntryNode = GetNode(0);
  PL_ASSERT_DEBUG(pEntryNode != nullptr &&
                      pEntryNode->m_Type == plVisualScriptNodeDescription::Type::MessageHandler ||
                    pEntryNode->m_Type == plVisualScriptNodeDescription::Type::MessageHandler_Coroutine ||
                    pEntryNode->m_Type == plVisualScriptNodeDescription::Type::SendMessage,
    "Entry node is invalid or not a message handler");

  auto& userData = pEntryNode->GetUserData<NodeUserData_TypeAndProperties>();

  plScriptMessageDesc desc;
  desc.m_pType = userData.m_pType;
  desc.m_Properties = plMakeArrayPtr(userData.m_Properties, userData.m_uiNumProperties);
  return desc;
}

//////////////////////////////////////////////////////////////////////////

plCVarInt cvar_MaxNodeExecutions("VisualScript.MaxNodeExecutions", 100000, plCVarFlags::Default, "The maximum number of nodes executed within a script invocation");

plVisualScriptExecutionContext::plVisualScriptExecutionContext(const plSharedPtr<const plVisualScriptGraphDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

plVisualScriptExecutionContext::~plVisualScriptExecutionContext()
{
  Deinitialize();
}

void plVisualScriptExecutionContext::Initialize(plVisualScriptInstance& inout_instance, plVisualScriptDataStorage& inout_localDataStorage, plArrayPtr<plVariant> arguments)
{
  m_pInstance = &inout_instance;

  m_DataStorage[DataOffset::Source::Local] = &inout_localDataStorage;
  m_DataStorage[DataOffset::Source::Instance] = inout_instance.GetInstanceDataStorage();
  m_DataStorage[DataOffset::Source::Constant] = inout_instance.GetConstantDataStorage();

  auto pNode = m_pDesc->GetNode(0);
  PL_ASSERT_DEV(plVisualScriptNodeDescription::Type::IsEntry(pNode->m_Type), "Invalid entry node");

  for (plUInt32 i = 0; i < arguments.GetCount(); ++i)
  {
    SetDataFromVariant(pNode->GetOutputDataOffset(i), arguments[i]);
  }

  m_uiCurrentNode = pNode->GetExecutionIndex(0);
}

void plVisualScriptExecutionContext::Deinitialize()
{
  // 0x1 is a marker value to indicate that we are in a yield
  if (m_pCurrentCoroutine > reinterpret_cast<plScriptCoroutine*>(0x1))
  {
    auto pModule = m_pInstance->GetWorld()->GetOrCreateModule<plScriptWorldModule>();
    pModule->StopAndDeleteCoroutine(m_pCurrentCoroutine->GetHandle());
    m_pCurrentCoroutine = nullptr;
  }
}

plVisualScriptExecutionContext::ExecResult plVisualScriptExecutionContext::Execute(plTime deltaTimeSinceLastExecution)
{
  PL_ASSERT_DEV(m_pInstance != nullptr, "Invalid instance");
  ++m_uiExecutionCounter;
  m_DeltaTimeSinceLastExecution = deltaTimeSinceLastExecution;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  plUInt32 uiCounter = 0;
#endif

  auto pNode = m_pDesc->GetNode(m_uiCurrentNode);
  while (pNode != nullptr)
  {
    ExecResult result = pNode->m_Function(*this, *pNode);
    if (result.m_NextExecAndState < ExecResult::State::Completed)
    {
      return result;
    }

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
    ++uiCounter;
    if (uiCounter >= cvar_MaxNodeExecutions)
    {
      plLog::Error("Maximum node executions ({}) reached, execution will be aborted. Does the script contain an infinite loop?", cvar_MaxNodeExecutions);
      return ExecResult::Error();
    }
#endif

    m_uiCurrentNode = pNode->GetExecutionIndex(result.m_NextExecAndState);
    m_pCurrentCoroutine = nullptr;

    pNode = m_pDesc->GetNode(m_uiCurrentNode);
  }

  return ExecResult::RunNext(0);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plVisualScriptSendMessageMode, 1)
  PL_ENUM_CONSTANTS(plVisualScriptSendMessageMode::Direct, plVisualScriptSendMessageMode::Recursive, plVisualScriptSendMessageMode::Event)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on
