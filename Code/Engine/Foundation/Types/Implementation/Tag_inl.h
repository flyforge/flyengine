#pragma once

plTag::plTag()


  = default;

bool plTag::operator==(const plTag& rhs) const
{
  return m_sTagString == rhs.m_sTagString;
}

bool plTag::operator!=(const plTag& rhs) const
{
  return m_sTagString != rhs.m_sTagString;
}

bool plTag::operator<(const plTag& rhs) const
{
  return m_sTagString < rhs.m_sTagString;
}

const plString& plTag::GetTagString() const
{
  return m_sTagString.GetString();
}

bool plTag::IsValid() const
{
  return m_uiBlockIndex != 0xFFFFFFFEu;
}
