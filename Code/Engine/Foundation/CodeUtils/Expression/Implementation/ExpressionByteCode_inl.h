
PL_ALWAYS_INLINE const plExpressionByteCode::StorageType* plExpressionByteCode::GetByteCodeStart() const
{
  return m_pByteCode;
}

PL_ALWAYS_INLINE const plExpressionByteCode::StorageType* plExpressionByteCode::GetByteCodeEnd() const
{
  return m_pByteCode + m_uiByteCodeCount;
}

PL_ALWAYS_INLINE plArrayPtr<const plExpressionByteCode::StorageType> plExpressionByteCode::GetByteCode() const
{
  return plMakeArrayPtr(m_pByteCode, m_uiByteCodeCount);
}

PL_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetNumInstructions() const
{
  return m_uiNumInstructions;
}

PL_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetNumTempRegisters() const
{
  return m_uiNumTempRegisters;
}

PL_ALWAYS_INLINE plArrayPtr<const plExpression::StreamDesc> plExpressionByteCode::GetInputs() const
{
  return plMakeArrayPtr(m_pInputs, m_uiNumInputs);
}

PL_ALWAYS_INLINE plArrayPtr<const plExpression::StreamDesc> plExpressionByteCode::GetOutputs() const
{
  return plMakeArrayPtr(m_pOutputs, m_uiNumOutputs);
}

PL_ALWAYS_INLINE plArrayPtr<const plExpression::FunctionDesc> plExpressionByteCode::GetFunctions() const
{
  return plMakeArrayPtr(m_pFunctions, m_uiNumFunctions);
}

// static
PL_ALWAYS_INLINE plExpressionByteCode::OpCode::Enum plExpressionByteCode::GetOpCode(const StorageType*& ref_pByteCode)
{
  plUInt32 uiOpCode = *ref_pByteCode;
  ++ref_pByteCode;
  return static_cast<OpCode::Enum>((uiOpCode >= 0 && uiOpCode < OpCode::Count) ? uiOpCode : 0);
}

// static
PL_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetRegisterIndex(const StorageType*& ref_pByteCode)
{
  plUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
PL_ALWAYS_INLINE plExpression::Register plExpressionByteCode::GetConstant(const StorageType*& ref_pByteCode)
{
  plExpression::Register r;
  r.i = plSimdVec4i(*ref_pByteCode);
  ++ref_pByteCode;
  return r;
}

// static
PL_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetFunctionIndex(const StorageType*& ref_pByteCode)
{
  plUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
PL_ALWAYS_INLINE plUInt32 plExpressionByteCode::GetFunctionArgCount(const StorageType*& ref_pByteCode)
{
  plUInt32 uiArgCount = *ref_pByteCode;
  ++ref_pByteCode;
  return uiArgCount;
}
