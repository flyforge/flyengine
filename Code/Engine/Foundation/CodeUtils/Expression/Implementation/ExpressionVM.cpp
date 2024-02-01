#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionAST.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/CodeUtils/Expression/Implementation/ExpressionVMOperations.h>
#include <Foundation/Logging/Log.h>

plExpressionVM::plExpressionVM()
{
  RegisterDefaultFunctions();
}
plExpressionVM::~plExpressionVM() = default;

void plExpressionVM::RegisterFunction(const plExpressionFunction& func)
{
  PL_ASSERT_DEV(func.m_Desc.m_uiNumRequiredInputs <= func.m_Desc.m_InputTypes.GetCount(), "Not enough input types defined. {} inputs are required but only {} types given.", func.m_Desc.m_uiNumRequiredInputs, func.m_Desc.m_InputTypes.GetCount());

  plUInt32 uiFunctionIndex = m_Functions.GetCount();
  m_FunctionNamesToIndex.Insert(func.m_Desc.GetMangledName(), uiFunctionIndex);

  m_Functions.PushBack(func);
}

void plExpressionVM::UnregisterFunction(const plExpressionFunction& func)
{
  plUInt32 uiFunctionIndex = 0;
  if (m_FunctionNamesToIndex.Remove(func.m_Desc.GetMangledName(), &uiFunctionIndex))
  {
    m_Functions.RemoveAtAndSwap(uiFunctionIndex);
    if (uiFunctionIndex != m_Functions.GetCount())
    {
      m_FunctionNamesToIndex[m_Functions[uiFunctionIndex].m_Desc.GetMangledName()] = uiFunctionIndex;
    }
  }
}

plResult plExpressionVM::Execute(const plExpressionByteCode& byteCode, plArrayPtr<const plProcessingStream> inputs,
  plArrayPtr<plProcessingStream> outputs, plUInt32 uiNumInstances, const plExpression::GlobalData& globalData, plBitflags<Flags> flags)
{
  if (flags.IsSet(Flags::ScalarizeStreams))
  {
    PL_SUCCEED_OR_RETURN(ScalarizeStreams(inputs, m_ScalarizedInputs));
    PL_SUCCEED_OR_RETURN(ScalarizeStreams(outputs, m_ScalarizedOutputs));

    inputs = m_ScalarizedInputs;
    outputs = m_ScalarizedOutputs;
  }
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  else
  {
    AreStreamsScalarized(inputs).AssertSuccess("Input streams are not scalarized");
    AreStreamsScalarized(outputs).AssertSuccess("Output streams are not scalarized");
  }
#endif

  PL_SUCCEED_OR_RETURN(MapStreams(byteCode.GetInputs(), inputs, "Input", uiNumInstances, flags, m_MappedInputs));
  PL_SUCCEED_OR_RETURN(MapStreams(byteCode.GetOutputs(), outputs, "Output", uiNumInstances, flags, m_MappedOutputs));

  PL_SUCCEED_OR_RETURN(MapFunctions(byteCode.GetFunctions(), globalData));

  const plUInt32 uiTotalNumRegisters = byteCode.GetNumTempRegisters() * ((uiNumInstances + 3) / 4);
  m_Registers.SetCountUninitialized(uiTotalNumRegisters);

  // Execute bytecode
  const plExpressionByteCode::StorageType* pByteCode = byteCode.GetByteCodeStart();
  const plExpressionByteCode::StorageType* pByteCodeEnd = byteCode.GetByteCodeEnd();

  ExecutionContext context;
  context.m_pRegisters = m_Registers.GetData();
  context.m_uiNumInstances = uiNumInstances;
  context.m_uiNumSimd4Instances = (uiNumInstances + 3) / 4;
  context.m_Inputs = m_MappedInputs;
  context.m_Outputs = m_MappedOutputs;
  context.m_Functions = m_MappedFunctions;
  context.m_pGlobalData = &globalData;

  while (pByteCode < pByteCodeEnd)
  {
    plExpressionByteCode::OpCode::Enum opCode = plExpressionByteCode::GetOpCode(pByteCode);

    OpFunc func = s_Simd4Funcs[opCode];
    if (func != nullptr)
    {
      func(pByteCode, context);
    }
    else
    {
      PL_ASSERT_NOT_IMPLEMENTED;
      plLog::Error("Unknown OpCode '{}'. Execution aborted.", opCode);
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

void plExpressionVM::RegisterDefaultFunctions()
{
  RegisterFunction(plDefaultExpressionFunctions::s_RandomFunc);
  RegisterFunction(plDefaultExpressionFunctions::s_PerlinNoiseFunc);
}

plResult plExpressionVM::ScalarizeStreams(plArrayPtr<const plProcessingStream> streams, plDynamicArray<plProcessingStream>& out_ScalarizedStreams)
{
  out_ScalarizedStreams.Clear();

  for (auto& stream : streams)
  {
    const plUInt32 uiNumElements = plExpressionAST::DataType::GetElementCount(plExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements == 1)
    {
      out_ScalarizedStreams.PushBack(stream);
    }
    else
    {
      plStringBuilder sNewName;
      plHashedString sNewNameHashed;
      auto data = plMakeArrayPtr((plUInt8*)(stream.GetData()), static_cast<plUInt32>(stream.GetDataSize()));
      auto elementDataType = static_cast<plProcessingStream::DataType>((plUInt32)stream.GetDataType() & ~3u);

      for (plUInt32 i = 0; i < uiNumElements; ++i)
      {
        sNewName.Set(stream.GetName(), ".", plExpressionAST::VectorComponent::GetName(static_cast<plExpressionAST::VectorComponent::Enum>(i)));
        sNewNameHashed.Assign(sNewName);

        auto newData = data.GetSubArray(i * plProcessingStream::GetDataTypeSize(elementDataType));

        out_ScalarizedStreams.PushBack(plProcessingStream(sNewNameHashed, newData, elementDataType, stream.GetElementStride()));
      }
    }
  }

  return PL_SUCCESS;
}

plResult plExpressionVM::AreStreamsScalarized(plArrayPtr<const plProcessingStream> streams)
{
  for (auto& stream : streams)
  {
    const plUInt32 uiNumElements = plExpressionAST::DataType::GetElementCount(plExpressionAST::DataType::FromStreamType(stream.GetDataType()));
    if (uiNumElements > 1)
    {
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}


plResult plExpressionVM::ValidateStream(const plProcessingStream& stream, const plExpression::StreamDesc& streamDesc, plStringView sStreamType, plUInt32 uiNumInstances)
{
  // verify stream data type
  if (stream.GetDataType() != streamDesc.m_DataType)
  {
    plLog::Error("{} stream '{}' expects data of type '{}' or a compatible type. Given type '{}' is not compatible.", sStreamType, streamDesc.m_sName, plProcessingStream::GetDataTypeName(streamDesc.m_DataType), plProcessingStream::GetDataTypeName(stream.GetDataType()));
    return PL_FAILURE;
  }

  // verify stream size
  plUInt32 uiElementSize = stream.GetElementSize();
  plUInt32 uiExpectedSize = stream.GetElementStride() * (uiNumInstances - 1) + uiElementSize;

  if (stream.GetDataSize() < uiExpectedSize)
  {
    plLog::Error("{} stream '{}' data size must be {} bytes or more. Only {} bytes given", sStreamType, streamDesc.m_sName, uiExpectedSize, stream.GetDataSize());
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

template <typename T>
plResult plExpressionVM::MapStreams(plArrayPtr<const plExpression::StreamDesc> streamDescs, plArrayPtr<T> streams, plStringView sStreamType, plUInt32 uiNumInstances, plBitflags<Flags> flags, plDynamicArray<T*>& out_MappedStreams)
{
  out_MappedStreams.Clear();
  out_MappedStreams.Reserve(streamDescs.GetCount());

  if (flags.IsSet(Flags::MapStreamsByName))
  {
    for (auto& streamDesc : streamDescs)
    {
      bool bFound = false;

      for (plUInt32 i = 0; i < streams.GetCount(); ++i)
      {
        auto& stream = streams[i];
        if (stream.GetName() == streamDesc.m_sName)
        {
          PL_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));

          out_MappedStreams.PushBack(&stream);
          bFound = true;
          break;
        }
      }

      if (!bFound)
      {
        plLog::Error("Bytecode expects an {} stream '{}'", sStreamType, streamDesc.m_sName);
        return PL_FAILURE;
      }
    }
  }
  else
  {
    if (streams.GetCount() != streamDescs.GetCount())
      return PL_FAILURE;

    for (plUInt32 i = 0; i < streams.GetCount(); ++i)
    {
      auto& stream = streams.GetPtr()[i];

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
      auto& streamDesc = streamDescs.GetPtr()[i];
      PL_SUCCEED_OR_RETURN(ValidateStream(stream, streamDesc, sStreamType, uiNumInstances));
#endif

      out_MappedStreams.PushBack(&stream);
    }
  }

  return PL_SUCCESS;
}

plResult plExpressionVM::MapFunctions(plArrayPtr<const plExpression::FunctionDesc> functionDescs, const plExpression::GlobalData& globalData)
{
  m_MappedFunctions.Clear();
  m_MappedFunctions.Reserve(functionDescs.GetCount());

  for (auto& functionDesc : functionDescs)
  {
    plUInt32 uiFunctionIndex = 0;
    if (!m_FunctionNamesToIndex.TryGetValue(functionDesc.m_sName, uiFunctionIndex))
    {
      plLog::Error("Bytecode expects a function called '{0}' but it was not registered for this VM", functionDesc.m_sName);
      return PL_FAILURE;
    }

    auto& registeredFunction = m_Functions[uiFunctionIndex];

    // verify signature
    if (functionDesc.m_InputTypes != registeredFunction.m_Desc.m_InputTypes || functionDesc.m_OutputType != registeredFunction.m_Desc.m_OutputType)
    {
      plLog::Error("Signature for registered function '{}' does not match the expected signature from bytecode", functionDesc.m_sName);
      return PL_FAILURE;
    }

    if (registeredFunction.m_ValidateGlobalDataFunc != nullptr)
    {
      if (registeredFunction.m_ValidateGlobalDataFunc(globalData).Failed())
      {
        plLog::Error("Global data validation for function '{0}' failed.", functionDesc.m_sName);
        return PL_FAILURE;
      }
    }

    m_MappedFunctions.PushBack(&registeredFunction);
  }

  return PL_SUCCESS;
}
