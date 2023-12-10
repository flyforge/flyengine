
PLASMA_ALWAYS_INLINE const plExpressionByteCode::StorageType* plExpressionByteCode::GetByteCode() const
{
  return m_ByteCode.GetData();
}

PLASMA_ALWAYS_INLINE const plExpressionByteCode::StorageType* plExpressionByteCode::GetByteCodeEnd() const
{
  return m_ByteCode.GetData() + m_ByteCode.GetCount();
}

PLASMA_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetNumInstructions() const
{
  return m_uiNumInstructions;
}

PLASMA_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetNumTempRegisters() const
{
  return m_uiNumTempRegisters;
}

PLASMA_ALWAYS_INLINE plArrayPtr<const plExpression::StreamDesc> plExpressionByteCode::GetInputs() const
{
  return m_Inputs;
}

PLASMA_ALWAYS_INLINE plArrayPtr<const plExpression::StreamDesc> plExpressionByteCode::GetOutputs() const
{
  return m_Outputs;
}

PLASMA_ALWAYS_INLINE plArrayPtr<const plExpression::FunctionDesc> plExpressionByteCode::GetFunctions() const
{
  return m_Functions;
}

// static
PLASMA_ALWAYS_INLINE plExpressionByteCode::OpCode::Enum plExpressionByteCode::GetOpCode(const StorageType*& ref_pByteCode)
{
  plUInt32 uiOpCode = *ref_pByteCode;
  ++ref_pByteCode;
  return static_cast<OpCode::Enum>((uiOpCode >= 0 && uiOpCode < OpCode::Count) ? uiOpCode : 0);
}

// static
PLASMA_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetRegisterIndex(const StorageType*& ref_pByteCode)
{
  plUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
PLASMA_ALWAYS_INLINE plExpression::Register plExpressionByteCode::GetConstant(const StorageType*& ref_pByteCode)
{
  plExpression::Register r;
  r.i = plSimdVec4i(*ref_pByteCode);
  ++ref_pByteCode;
  return r;
}

// static
PLASMA_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetFunctionIndex(const StorageType*& ref_pByteCode)
{
  plUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
PLASMA_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetFunctionArgCount(const StorageType*& ref_pByteCode)
{
  plUInt32 uiArgCount = *ref_pByteCode;
  ++ref_pByteCode;
  return uiArgCount;
}
