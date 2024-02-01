#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicBitflags.h>

plMap<plString, plDynamicBitflags> plDynamicBitflags::s_DynamicBitflags;

void plDynamicBitflags::Clear()
{
  m_ValidValues.Clear();
}

void plDynamicBitflags::SetValueAndName(plUInt32 uiBitPos, plStringView sName)
{
  PL_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  auto it = m_ValidValues.FindOrAdd(PL_BIT(uiBitPos));
  it.Value() = sName;
}

void plDynamicBitflags::RemoveValue(plUInt32 uiBitPos)
{
  PL_ASSERT_DEV(uiBitPos < 64, "Only up to 64 bits is supported.");
  m_ValidValues.Remove(PL_BIT(uiBitPos));
}

bool plDynamicBitflags::IsValueValid(plUInt32 uiBitPos) const
{
  return m_ValidValues.Find(PL_BIT(uiBitPos)).IsValid();
}

bool plDynamicBitflags::TryGetValueName(plUInt32 uiBitPos, plStringView& out_sName) const
{
  auto it = m_ValidValues.Find(PL_BIT(uiBitPos));
  if (it.IsValid())
  {
    out_sName = it.Value();
    return true;
  }
  return false;
}

plDynamicBitflags& plDynamicBitflags::GetDynamicBitflags(plStringView sName)
{
  return s_DynamicBitflags[sName];
}
