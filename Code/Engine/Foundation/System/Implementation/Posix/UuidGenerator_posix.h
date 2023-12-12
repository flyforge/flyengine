#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <uuid/uuid.h>


PLASMA_CHECK_AT_COMPILETIME(sizeof(plUInt64) * 2 == sizeof(uuid_t));

void plUuid::CreateNewUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  plUInt64* uiUuidData = reinterpret_cast<plUInt64*>(uuid);

  m_uiHigh = uiUuidData[0];
  m_uiLow = uiUuidData[1];
}
