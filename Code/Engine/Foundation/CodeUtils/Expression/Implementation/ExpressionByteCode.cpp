#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

namespace
{
  static constexpr const char* s_szOpCodeNames[] = {
    "Nop",

    "",

    "AbsF_R",
    "AbsI_R",
    "SqrtF_R",

    "ExpF_R",
    "LnF_R",
    "Log2F_R",
    "Log2I_R",
    "Log10F_R",
    "Pow2F_R",

    "SinF_R",
    "CosF_R",
    "TanF_R",

    "ASinF_R",
    "ACosF_R",
    "ATanF_R",

    "RoundF_R",
    "FloorF_R",
    "CeilF_R",
    "TruncF_R",

    "NotB_R",
    "NotI_R",

    "IToF_R",
    "FToI_R",

    "",
    "",

    "AddF_RR",
    "AddI_RR",

    "SubF_RR",
    "SubI_RR",

    "MulF_RR",
    "MulI_RR",

    "DivF_RR",
    "DivI_RR",

    "MinF_RR",
    "MinI_RR",

    "MaxF_RR",
    "MaxI_RR",

    "ShlI_RR",
    "ShrI_RR",
    "AndI_RR",
    "XorI_RR",
    "OrI_RR",

    "EqF_RR",
    "EqI_RR",
    "EqB_RR",

    "NEqF_RR",
    "NEqI_RR",
    "NEqB_RR",

    "LtF_RR",
    "LtI_RR",

    "LEqF_RR",
    "LEqI_RR",

    "GtF_RR",
    "GtI_RR",

    "GEqF_RR",
    "GEqI_RR",

    "AndB_RR",
    "OrB_RR",

    "",
    "",

    "AddF_RC",
    "AddI_RC",

    "SubF_RC",
    "SubI_RC",

    "MulF_RC",
    "MulI_RC",

    "DivF_RC",
    "DivI_RC",

    "MinF_RC",
    "MinI_RC",

    "MaxF_RC",
    "MaxI_RC",

    "ShlI_RC",
    "ShrI_RC",
    "AndI_RC",
    "XorI_RC",
    "OrI_RC",

    "EqF_RC",
    "EqI_RC",
    "EqB_RC",

    "NEqF_RC",
    "NEqI_RC",
    "NEqB_RC",

    "LtF_RC",
    "LtI_RC",

    "LEqF_RC",
    "LEqI_RC",

    "GtF_RC",
    "GtI_RC",

    "GEqF_RC",
    "GEqI_RC",

    "AndB_RC",
    "OrB_RC",

    "",
    "",

    "SelF_RRR",
    "SelI_RRR",
    "SelB_RRR",

    "",
    "",

    "MovX_R",
    "MovX_C",
    "LoadF",
    "LoadI",
    "StoreF",
    "StoreI",

    "Call",

    "",
  };

  static_assert(PL_ARRAY_SIZE(s_szOpCodeNames) == plExpressionByteCode::OpCode::Count);
  static_assert(plExpressionByteCode::OpCode::LastBinary - plExpressionByteCode::OpCode::FirstBinary == plExpressionByteCode::OpCode::LastBinaryWithConstant - plExpressionByteCode::OpCode::FirstBinaryWithConstant);


  static constexpr plUInt32 GetMaxOpCodeLength()
  {
    plUInt32 uiMaxLength = 0;
    for (plUInt32 i = 0; i < PL_ARRAY_SIZE(s_szOpCodeNames); ++i)
    {
      uiMaxLength = plMath::Max(uiMaxLength, plStringUtils::GetStringElementCount(s_szOpCodeNames[i]));
    }
    return uiMaxLength;
  }

  static constexpr plUInt32 s_uiMaxOpCodeLength = GetMaxOpCodeLength();

} // namespace

const char* plExpressionByteCode::OpCode::GetName(Enum code)
{
  PL_ASSERT_DEBUG(code >= 0 && code < PL_ARRAY_SIZE(s_szOpCodeNames), "Out of bounds access");
  return s_szOpCodeNames[code];
}

//////////////////////////////////////////////////////////////////////////

//clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plExpressionByteCode, plNoBase, 1, plRTTINoAllocator)
PL_END_STATIC_REFLECTED_TYPE;
//clang-format on

plExpressionByteCode::plExpressionByteCode() = default;

plExpressionByteCode::plExpressionByteCode(const plExpressionByteCode& other)
{
  *this = other;
}

plExpressionByteCode::~plExpressionByteCode()
{
  Clear();
}

void plExpressionByteCode::operator=(const plExpressionByteCode& other)
{
  Clear();
  Init(other.GetByteCode(), other.GetInputs(), other.GetOutputs(), other.GetFunctions(), other.GetNumTempRegisters(), other.GetNumInstructions());
}

bool plExpressionByteCode::operator==(const plExpressionByteCode& other) const
{
  return GetByteCode() == other.GetByteCode() &&
         GetInputs() == other.GetInputs() &&
         GetOutputs() == other.GetOutputs() &&
         GetFunctions() == other.GetFunctions();
}

void plExpressionByteCode::Clear()
{
  plMemoryUtils::Destruct(m_pInputs, m_uiNumInputs);
  plMemoryUtils::Destruct(m_pOutputs, m_uiNumOutputs);
  plMemoryUtils::Destruct(m_pFunctions, m_uiNumFunctions);

  m_pInputs = nullptr;
  m_pOutputs = nullptr;
  m_pFunctions = nullptr;
  m_pByteCode = nullptr;

  m_uiByteCodeCount = 0;
  m_uiNumInputs = 0;
  m_uiNumOutputs = 0;
  m_uiNumFunctions = 0;

  m_uiNumTempRegisters = 0;
  m_uiNumInstructions = 0;

  m_Data.Clear();
}

void plExpressionByteCode::Disassemble(plStringBuilder& out_sDisassembly) const
{
  out_sDisassembly.Append("// Inputs:\n");
  for (plUInt32 i = 0; i < m_uiNumInputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pInputs[i].m_sName, plProcessingStream::GetDataTypeName(m_pInputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Outputs:\n");
  for (plUInt32 i = 0; i < m_uiNumOutputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pOutputs[i].m_sName, plProcessingStream::GetDataTypeName(m_pOutputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Functions:\n");
  for (plUInt32 i = 0; i < m_uiNumFunctions; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {} {}(", i, plExpression::RegisterType::GetName(m_pFunctions[i].m_OutputType), m_pFunctions[i].m_sName);
    const plUInt32 uiNumArguments = m_pFunctions[i].m_InputTypes.GetCount();
    for (plUInt32 j = 0; j < uiNumArguments; ++j)
    {
      out_sDisassembly.Append(plExpression::RegisterType::GetName(m_pFunctions[i].m_InputTypes[j]));
      if (j < uiNumArguments - 1)
      {
        out_sDisassembly.Append(", ");
      }
    }
    out_sDisassembly.Append(")\n");
  }

  out_sDisassembly.AppendFormat("\n// Temp Registers: {}\n", GetNumTempRegisters());
  out_sDisassembly.AppendFormat("// Instructions: {}\n\n", GetNumInstructions());

  auto AppendConstant = [](plUInt32 x, plStringBuilder& out_sString)
  {
    out_sString.AppendFormat("0x{}({})", plArgU(x, 8, true, 16), plArgF(*reinterpret_cast<float*>(&x), 6));
  };

  const StorageType* pByteCode = GetByteCodeStart();
  const StorageType* pByteCodeEnd = GetByteCodeEnd();

  while (pByteCode < pByteCodeEnd)
  {
    OpCode::Enum opCode = GetOpCode(pByteCode);
    {
      const char* szOpCode = OpCode::GetName(opCode);
      plUInt32 uiOpCodeLength = plStringUtils::GetStringElementCount(szOpCode);

      out_sDisassembly.Append(szOpCode);
      for (plUInt32 i = uiOpCodeLength; i < s_uiMaxOpCodeLength + 1; ++i)
      {
        out_sDisassembly.Append(" ");
      }
    }

    if (opCode > OpCode::FirstUnary && opCode < OpCode::LastUnary)
    {
      plUInt32 r = GetRegisterIndex(pByteCode);
      plUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{}\n", r, x);
    }
    else if (opCode > OpCode::FirstBinary && opCode < OpCode::LastBinary)
    {
      plUInt32 r = GetRegisterIndex(pByteCode);
      plUInt32 a = GetRegisterIndex(pByteCode);
      plUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{}\n", r, a, b);
    }
    else if (opCode > OpCode::FirstBinaryWithConstant && opCode < OpCode::LastBinaryWithConstant)
    {
      plUInt32 r = GetRegisterIndex(pByteCode);
      plUInt32 a = GetRegisterIndex(pByteCode);
      plUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} ", r, a);
      AppendConstant(b, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode > OpCode::FirstTernary && opCode < OpCode::LastTernary)
    {
      plUInt32 r = GetRegisterIndex(pByteCode);
      plUInt32 a = GetRegisterIndex(pByteCode);
      plUInt32 b = GetRegisterIndex(pByteCode);
      plUInt32 c = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{} r{}\n", r, a, b, c);
    }
    else if (opCode == OpCode::MovX_C)
    {
      plUInt32 r = GetRegisterIndex(pByteCode);
      plUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} ", r);
      AppendConstant(x, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode == OpCode::LoadF || opCode == OpCode::LoadI)
    {
      plUInt32 r = GetRegisterIndex(pByteCode);
      plUInt32 i = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} i{}({})\n", r, i, m_pInputs[i].m_sName);
    }
    else if (opCode == OpCode::StoreF || opCode == OpCode::StoreI)
    {
      plUInt32 o = GetRegisterIndex(pByteCode);
      plUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("o{}({}) r{}\n", o, m_pOutputs[o].m_sName, r);
    }
    else if (opCode == OpCode::Call)
    {
      plUInt32 uiIndex = GetFunctionIndex(pByteCode);
      const char* szName = m_pFunctions[uiIndex].m_sName;

      plStringBuilder sName;
      if (plStringUtils::IsNullOrEmpty(szName))
      {
        sName.SetFormat("Unknown_{0}", uiIndex);
      }
      else
      {
        sName = szName;
      }

      plUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("{1} r{2}", sName, r);

      plUInt32 uiNumArgs = GetFunctionArgCount(pByteCode);
      for (plUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
      {
        plUInt32 x = GetRegisterIndex(pByteCode);
        out_sDisassembly.AppendFormat(" r{0}", x);
      }

      out_sDisassembly.Append("\n");
    }
    else
    {
      PL_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

static constexpr plTypeVersion s_uiByteCodeVersion = 6;

plResult plExpressionByteCode::Save(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiByteCodeVersion);

  plUInt32 uiDataSize = static_cast<plUInt32>(m_Data.GetByteBlobPtr().GetCount());

  inout_stream << uiDataSize;

  inout_stream << m_uiNumInputs;
  for (auto& input : GetInputs())
  {
    PL_SUCCEED_OR_RETURN(input.Serialize(inout_stream));
  }

  inout_stream << m_uiNumOutputs;
  for (auto& output : GetOutputs())
  {
    PL_SUCCEED_OR_RETURN(output.Serialize(inout_stream));
  }

  inout_stream << m_uiNumFunctions;
  for (auto& function : GetFunctions())
  {
    PL_SUCCEED_OR_RETURN(function.Serialize(inout_stream));
  }

  inout_stream << m_uiByteCodeCount;
  PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType)));

  inout_stream << m_uiNumTempRegisters;
  inout_stream << m_uiNumInstructions;

  return PL_SUCCESS;
}

plResult plExpressionByteCode::Load(plStreamReader& inout_stream, plByteArrayPtr externalMemory /*= plByteArrayPtr()*/)
{
  plTypeVersion version = inout_stream.ReadVersion(s_uiByteCodeVersion);
  if (version != s_uiByteCodeVersion)
  {
    plLog::Error("Invalid expression byte code version {}. Expected {}", version, s_uiByteCodeVersion);
    return PL_FAILURE;
  }

  plUInt32 uiDataSize = 0;
  inout_stream >> uiDataSize;

  void* pData = nullptr;
  if (externalMemory.IsEmpty())
  {
    m_Data.SetCountUninitialized(uiDataSize);
    m_Data.ZeroFill();
    pData = m_Data.GetByteBlobPtr().GetPtr();
  }
  else
  {
    if (externalMemory.GetCount() < uiDataSize)
    {
      plLog::Error("External memory is too small. Expected at least {} bytes but got {} bytes.", uiDataSize, externalMemory.GetCount());
      return PL_FAILURE;
    }

    if (plMemoryUtils::IsAligned(externalMemory.GetPtr(), PL_ALIGNMENT_OF(plExpression::StreamDesc)) == false)
    {
      plLog::Error("External memory is not properly aligned. Expected an alignment of at least {} bytes.", PL_ALIGNMENT_OF(plExpression::StreamDesc));
      return PL_FAILURE;
    }

    pData = externalMemory.GetPtr();
  }

  // Inputs
  {
    inout_stream >> m_uiNumInputs;
    m_pInputs = static_cast<plExpression::StreamDesc*>(pData);
    for (plUInt32 i = 0; i < m_uiNumInputs; ++i)
    {
      PL_SUCCEED_OR_RETURN(m_pInputs[i].Deserialize(inout_stream));
    }

    pData = plMemoryUtils::AddByteOffset(pData, GetInputs().ToByteArray().GetCount());
  }

  // Outputs
  {
    inout_stream >> m_uiNumOutputs;
    m_pOutputs = static_cast<plExpression::StreamDesc*>(pData);
    for (plUInt32 i = 0; i < m_uiNumOutputs; ++i)
    {
      PL_SUCCEED_OR_RETURN(m_pOutputs[i].Deserialize(inout_stream));
    }

    pData = plMemoryUtils::AddByteOffset(pData, GetOutputs().ToByteArray().GetCount());
  }

  // Functions
  {
    pData = plMemoryUtils::AlignForwards(pData, PL_ALIGNMENT_OF(plExpression::FunctionDesc));

    inout_stream >> m_uiNumFunctions;
    m_pFunctions = static_cast<plExpression::FunctionDesc*>(pData);
    for (plUInt32 i = 0; i < m_uiNumFunctions; ++i)
    {
      PL_SUCCEED_OR_RETURN(m_pFunctions[i].Deserialize(inout_stream));
    }

    pData = plMemoryUtils::AddByteOffset(pData, GetFunctions().ToByteArray().GetCount());
  }

  // ByteCode
  {
    pData = plMemoryUtils::AlignForwards(pData, PL_ALIGNMENT_OF(StorageType));

    inout_stream >> m_uiByteCodeCount;
    m_pByteCode = static_cast<StorageType*>(pData);
    inout_stream.ReadBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType));
  }

  inout_stream >> m_uiNumTempRegisters;
  inout_stream >> m_uiNumInstructions;

  return PL_SUCCESS;
}

void plExpressionByteCode::Init(plArrayPtr<const StorageType> byteCode, plArrayPtr<const plExpression::StreamDesc> inputs, plArrayPtr<const plExpression::StreamDesc> outputs, plArrayPtr<const plExpression::FunctionDesc> functions, plUInt32 uiNumTempRegisters, plUInt32 uiNumInstructions)
{
  plUInt32 uiOutputsOffset = 0;
  plUInt32 uiFunctionsOffset = 0;
  plUInt32 uiByteCodeOffset = 0;

  plUInt32 uiDataSize = 0;
  uiDataSize += inputs.ToByteArray().GetCount();
  uiOutputsOffset = uiDataSize;
  uiDataSize += outputs.ToByteArray().GetCount();

  uiDataSize = plMemoryUtils::AlignSize<plUInt32>(uiDataSize, PL_ALIGNMENT_OF(plExpression::FunctionDesc));
  uiFunctionsOffset = uiDataSize;
  uiDataSize += functions.ToByteArray().GetCount();

  uiDataSize = plMemoryUtils::AlignSize<plUInt32>(uiDataSize, PL_ALIGNMENT_OF(StorageType));
  uiByteCodeOffset = uiDataSize;
  uiDataSize += byteCode.ToByteArray().GetCount();

  m_Data.SetCountUninitialized(uiDataSize);
  m_Data.ZeroFill();

  void* pData = m_Data.GetByteBlobPtr().GetPtr();

  PL_ASSERT_DEV(inputs.GetCount() < plSmallInvalidIndex, "Too many inputs");
  m_pInputs = static_cast<plExpression::StreamDesc*>(pData);
  m_uiNumInputs = static_cast<plUInt16>(inputs.GetCount());
  plMemoryUtils::Copy(m_pInputs, inputs.GetPtr(), m_uiNumInputs);

  PL_ASSERT_DEV(outputs.GetCount() < plSmallInvalidIndex, "Too many outputs");
  m_pOutputs = static_cast<plExpression::StreamDesc*>(plMemoryUtils::AddByteOffset(pData, uiOutputsOffset));
  m_uiNumOutputs = static_cast<plUInt16>(outputs.GetCount());
  plMemoryUtils::Copy(m_pOutputs, outputs.GetPtr(), m_uiNumOutputs);

  PL_ASSERT_DEV(functions.GetCount() < plSmallInvalidIndex, "Too many functions");
  m_pFunctions = static_cast<plExpression::FunctionDesc*>(plMemoryUtils::AddByteOffset(pData, uiFunctionsOffset));
  m_uiNumFunctions = static_cast<plUInt16>(functions.GetCount());
  plMemoryUtils::Copy(m_pFunctions, functions.GetPtr(), m_uiNumFunctions);

  m_pByteCode = static_cast<StorageType*>(plMemoryUtils::AddByteOffset(pData, uiByteCodeOffset));
  m_uiByteCodeCount = byteCode.GetCount();
  plMemoryUtils::Copy(m_pByteCode, byteCode.GetPtr(), m_uiByteCodeCount);

  PL_ASSERT_DEV(uiNumTempRegisters < plSmallInvalidIndex, "Too many temp registers");
  m_uiNumTempRegisters = static_cast<plUInt16>(uiNumTempRegisters);
  m_uiNumInstructions = uiNumInstructions;
}
