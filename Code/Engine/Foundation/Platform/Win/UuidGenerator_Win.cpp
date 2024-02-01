#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#  include <Foundation/Types/Uuid.h>

#  include <combaseapi.h>
#  include <rpc.h>

PL_CHECK_AT_COMPILETIME(sizeof(plUInt64) * 2 == sizeof(UUID));

plUuid plUuid::MakeUuid()
{
  plUInt64 uiUuidData[2];

  // this works on desktop Windows
  // UuidCreate(reinterpret_cast<UUID*>(uiUuidData));

  // this also works on UWP
  GUID* guid = reinterpret_cast<GUID*>(&uiUuidData[0]);
  HRESULT hr = CoCreateGuid(guid);
  PL_IGNORE_UNUSED(hr);
  PL_ASSERT_DEBUG(SUCCEEDED(hr), "CoCreateGuid failed, guid might be invalid!");

  return plUuid(uiUuidData[1], uiUuidData[0]);
}

#endif


