#include <Foundation/FoundationPCH.h>

#include <Foundation/Types/Uuid.h>

plUuid plUuid::MakeStableUuidFromString(plStringView sString)
{
  plUuid NewUuid;
  NewUuid.m_uiLow = plHashingUtils::xxHash64String(sString);
  NewUuid.m_uiHigh = plHashingUtils::xxHash64String(sString, 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}

plUuid plUuid::MakeStableUuidFromInt(plInt64 iInt)
{
  plUuid NewUuid;
  NewUuid.m_uiLow = plHashingUtils::xxHash64(&iInt, sizeof(plInt64));
  NewUuid.m_uiHigh = plHashingUtils::xxHash64(&iInt, sizeof(plInt64), 0x7FFFFFFFFFFFFFE7u);

  return NewUuid;
}


