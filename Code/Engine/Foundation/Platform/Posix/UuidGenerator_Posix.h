#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Types/Uuid.h>

#if __has_include(<uuid/uuid.h>)
#  include <uuid/uuid.h>
#  define HAS_UUID 1
#else
// #  error "uuid.h does not exist on this distro."
#  define HAS_UUID 0
#endif

#if HAS_UUID

PL_CHECK_AT_COMPILETIME(sizeof(plUInt64) * 2 == sizeof(uuid_t));

plUuid plUuid::MakeUuid()
{
  uuid_t uuid;
  uuid_generate(uuid);

  plUInt64* uiUuidData = reinterpret_cast<plUInt64*>(uuid);

  return plUuid(uiUuidData[1], uiUuidData[0]);
}

#else

plUuid plUuid::MakeUuid()
{
  PL_REPORT_FAILURE("This distro doesn't have support for UUID generation.");
  return plUuid();
}

#endif
