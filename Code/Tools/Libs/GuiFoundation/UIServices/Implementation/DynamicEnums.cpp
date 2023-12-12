#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/UIServices/DynamicEnums.h>

plMap<plString, plDynamicEnum> plDynamicEnum::s_DynamicEnums;

void plDynamicEnum::Clear()
{
  m_ValidValues.Clear();
}

void plDynamicEnum::SetValueAndName(plInt32 iValue, const char* szNewName)
{
  m_ValidValues[iValue] = szNewName;
}

void plDynamicEnum::RemoveValue(plInt32 iValue)
{
  m_ValidValues.Remove(iValue);
}

bool plDynamicEnum::IsValueValid(plInt32 iValue) const
{
  return m_ValidValues.Find(iValue).IsValid();
}

const char* plDynamicEnum::GetValueName(plInt32 iValue) const
{
  auto it = m_ValidValues.Find(iValue);

  if (!it.IsValid())
    return "<invalid value>";

  return it.Value();
}

plDynamicEnum& plDynamicEnum::GetDynamicEnum(const char* szEnumName)
{
  return s_DynamicEnums[szEnumName];
}
