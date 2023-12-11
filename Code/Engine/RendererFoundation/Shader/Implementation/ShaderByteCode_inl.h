

const void* plGALShaderByteCode::GetByteCode() const
{
  if (m_ByteCode.IsEmpty())
    return nullptr;

  return &m_ByteCode[0];
}

plUInt32 plGALShaderByteCode::GetSize() const
{
  return m_ByteCode.GetCount();
}

bool plGALShaderByteCode::IsValid() const
{
  return !m_ByteCode.IsEmpty();
}