#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Basics/Platform/Android/AndroidJni.h>
#include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#include <android_native_app_glue.h>

plUuid plUuid::MakeUuid()
{
  plJniAttachment attachment;

  plJniClass uuidClass("java/util/UUID");
  PLASMA_ASSERT_DEBUG(!uuidClass.IsNull(), "UUID class not found.");
  plJniObject javaUuid = uuidClass.CallStatic<plJniObject>("randomUUID");
  jlong mostSignificant = javaUuid.Call<jlong>("getMostSignificantBits");
  jlong leastSignificant = javaUuid.Call<jlong>("getLeastSignificantBits");

  return plUuid(leastSignificant, mostSignificant);

  //#TODO maybe faster to read /proc/sys/kernel/random/uuid, but that can't be done via plOSFile
  // see https://stackoverflow.com/questions/11888055/include-uuid-h-into-android-ndk-project
}