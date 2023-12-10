#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>

///\brief The compression method to be used
enum class plCompressionMethod : plUInt16
{
  ZStd = 0 ///< Only available when ZStd support is enabled in the build (default)
};

/// \brief This namespace contains utilities which can be used to compress and decompress data.
namespace plCompressionUtils
{
  ///\brief Compresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  PLASMA_FOUNDATION_DLL plResult Compress(plArrayPtr<const plUInt8> uncompressedData, plCompressionMethod method, plDynamicArray<plUInt8>& out_data);

  ///\brief Decompresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  PLASMA_FOUNDATION_DLL plResult Decompress(plArrayPtr<const plUInt8> compressedData, plCompressionMethod method, plDynamicArray<plUInt8>& out_data);
} // namespace plCompressionUtils
