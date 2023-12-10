
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

namespace plStreamUtils
{
  /// \brief Reads all the remaining data in \a stream and appends it to \a destination.
  PLASMA_FOUNDATION_DLL void ReadAllAndAppend(plStreamReader& inout_stream, plDynamicArray<plUInt8>& ref_destination);

} // namespace plStreamUtils
