#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StreamUtils.h>

void plStreamUtils::ReadAllAndAppend(plStreamReader& inout_stream, plDynamicArray<plUInt8>& ref_destination)
{
  plUInt8 temp[1024 * 4];

  while (true)
  {
    const plUInt32 uiRead = (plUInt32)inout_stream.ReadBytes(temp, PL_ARRAY_SIZE(temp));

    if (uiRead == 0)
      return;

    ref_destination.PushBackRange(plArrayPtr<plUInt8>(temp, uiRead));
  }
}


