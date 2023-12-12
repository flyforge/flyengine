

const void* plGALShaderByteCode::GetByteCode() const
{
  if (m_Source.IsEmpty())
    return nullptr;

  return &m_Source[0];
}

plUInt32 plGALShaderByteCode::GetSize() const
{
  return m_Source.GetCount();
}

bool plGALShaderByteCode::IsValid() const
{
  return !m_Source.IsEmpty();
}
