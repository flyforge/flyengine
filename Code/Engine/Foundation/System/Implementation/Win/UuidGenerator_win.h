#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <combaseapi.h>
#include <rpc.h>

PLASMA_CHECK_AT_COMPILETIME(sizeof(plUInt64) * 2 == sizeof(UUID));

plUuid plUuid::MakeUuid()
{
  plUInt64 uiUuidData[2];

  // this works on desktop Windows
  // UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  // this also works on UWP
  GUID* guid = reinterpret_cast<GUID*>(&uiUuidData[0]);
  HRESULT hr = CoCreateGuid(guid);
  PLASMA_ASSERT_DEBUG(SUCCEEDED(hr), "CoCreateGuid failed, guid might be invalid!");

  return plUuid(uiUuidData[1], uiUuidData[0]);
}