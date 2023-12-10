#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <uuid/uuid.h>


PLASMA_CHECK_AT_COMPILETIME(sizeof(plUInt64) * 2 == sizeof(uuid_t));

plUuid plUuid::MakeUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  plUInt64* uiUuidData = reinterpret_cast<plUInt64*>(uuid);

  return plUuid(uiUuidData[1], uiUuidData[0]);
}
