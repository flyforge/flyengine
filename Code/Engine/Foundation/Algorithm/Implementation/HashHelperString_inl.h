
plUInt32 plHashHelperString_NoCase::Hash(plStringView value)
{
  plHybridArray<char, 256> temp;
  temp.SetCountUninitialized(value.GetElementCount());
  plMemoryUtils::Copy(temp.GetData(), value.GetStartPointer(), value.GetElementCount());
  const plUInt32 uiElemCount = plStringUtils::ToLowerString(temp.GetData(), temp.GetData() + value.GetElementCount());

  return plHashingUtils::StringHashTo32(plHashingUtils::xxHash64((void*)temp.GetData(), uiElemCount));
}

bool plHashHelperString_NoCase::Equal(plStringView lhs, plStringView rhs)
{
  return lhs.IsEqual_NoCase(rhs);
}
