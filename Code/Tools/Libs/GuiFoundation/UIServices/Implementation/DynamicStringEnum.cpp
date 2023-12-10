#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

plMap<plString, plDynamicStringEnum> plDynamicStringEnum::s_DynamicEnums;
plDelegate<void(plStringView sEnumName, plDynamicStringEnum& e)> plDynamicStringEnum::s_RequestUnknownCallback;

// static
plDynamicStringEnum& plDynamicStringEnum::GetDynamicEnum(plStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  if (!bExisted && s_RequestUnknownCallback.IsValid())
  {
    s_RequestUnknownCallback(sEnumName, it.Value());
  }

  return it.Value();
}

// static
plDynamicStringEnum& plDynamicStringEnum::CreateDynamicEnum(plStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  plDynamicStringEnum& e = it.Value();
  e.Clear();
  e.SetStorageFile(nullptr);

  return e;
}

// static
void plDynamicStringEnum::RemoveEnum(plStringView sEnumName)
{
  s_DynamicEnums.Remove(sEnumName);
}

void plDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void plDynamicStringEnum::AddValidValue(plStringView sValue, bool bSortValues /*= false*/)
{
  plString sNewValue = sValue;

  if (!m_ValidValues.Contains(sNewValue))
    m_ValidValues.PushBack(sNewValue);

  if (bSortValues)
    SortValues();
}

void plDynamicStringEnum::RemoveValue(plStringView sValue)
{
  m_ValidValues.RemoveAndCopy(sValue);
}

bool plDynamicStringEnum::IsValueValid(plStringView sValue) const
{
  return m_ValidValues.Contains(sValue);
}

void plDynamicStringEnum::SortValues()
{
  m_ValidValues.Sort();
}

void plDynamicStringEnum::ReadFromStorage()
{
  Clear();

  plStringBuilder sFile, tmp;

  plFileReader file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  sFile.ReadAll(file);

  plHybridArray<plStringView, 32> values;

  sFile.Split(false, values, "\n", "\r");

  for (auto val : values)
  {
    AddValidValue(val.GetData(tmp));
  }
}

void plDynamicStringEnum::SaveToStorage()
{
  if (m_sStorageFile.IsEmpty())
    return;

  plFileWriter file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  plStringBuilder tmp;

  for (const auto& val : m_ValidValues)
  {
    tmp.Set(val, "\n");
    file.WriteBytes(tmp.GetData(), tmp.GetElementCount()).IgnoreResult();
  }
}
