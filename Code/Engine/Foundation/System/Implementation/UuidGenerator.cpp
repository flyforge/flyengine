#include <Foundation/FoundationPCH.h>

#include <Foundation/Types/Uuid.h>


// Include inline file
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/UuidGenerator_win.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)
#  include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/UuidGenerator_posix.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Android/UuidGenerator_android.h>
#else
#  error "Uuid generation functions are not implemented on current platform"
#endif

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

PLASMA_STATICLINK_FILE(Foundation, Foundation_System_Implementation_UuidGenerator);
