#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/Compression.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  define ZSTD_STATIC_LINKING_ONLY // ZSTD_findDecompressedSize
#  include <zstd/zstd.h>
#endif

namespace plCompressionUtils
{
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  static plResult CompressZStd(plArrayPtr<const plUInt8> uncompressedData, plDynamicArray<plUInt8>& out_data)
  {
    size_t uiSizeBound = ZSTD_compressBound(uncompressedData.GetCount());
    if (uiSizeBound > plMath::MaxValue<plUInt32>())
    {
      plLog::Error("Can't compress since the output container can't hold enough elements ({0})", static_cast<plUInt64>(uiSizeBound));
      return PLASMA_FAILURE;
    }

    out_data.SetCountUninitialized(static_cast<plUInt32>(uiSizeBound));

    size_t const cSize = ZSTD_compress(out_data.GetData(), uiSizeBound, uncompressedData.GetPtr(), uncompressedData.GetCount(), 1);
    if (ZSTD_isError(cSize))
    {
      plLog::Error("Compression failed with error: '{0}'.", ZSTD_getErrorName(cSize));
      return PLASMA_FAILURE;
    }

    out_data.SetCount(static_cast<plUInt32>(cSize));

    return PLASMA_SUCCESS;
  }

  static plResult DecompressZStd(plArrayPtr<const plUInt8> compressedData, plDynamicArray<plUInt8>& out_data)
  {
    plUInt64 uiSize = ZSTD_findDecompressedSize(compressedData.GetPtr(), compressedData.GetCount());

    if (uiSize == ZSTD_CONTENTSIZE_ERROR)
    {
      plLog::Error("Can't decompress since it wasn't compressed with ZStd");
      return PLASMA_FAILURE;
    }
    else if (uiSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
      plLog::Error("Can't decompress since the original size can't be determined, was the data compressed using the streaming variant?");
      return PLASMA_FAILURE;
    }

    if (uiSize > plMath::MaxValue<plUInt32>())
    {
      plLog::Error("Can't compress since the output container can't hold enough elements ({0})", uiSize);
      return PLASMA_FAILURE;
    }

    out_data.SetCountUninitialized(static_cast<plUInt32>(uiSize));

    size_t const uiActualSize = ZSTD_decompress(out_data.GetData(), plMath::SafeConvertToSizeT(uiSize), compressedData.GetPtr(), compressedData.GetCount());

    if (uiActualSize != uiSize)
    {
      plLog::Error("Error during ZStd decompression: '{0}'.", ZSTD_getErrorName(uiActualSize));
      return PLASMA_FAILURE;
    }

    return PLASMA_SUCCESS;
  }
#endif

  plResult Compress(plArrayPtr<const plUInt8> uncompressedData, plCompressionMethod method, plDynamicArray<plUInt8>& out_data)
  {
    out_data.Clear();

    if (uncompressedData.IsEmpty())
    {
      return PLASMA_SUCCESS;
    }

    switch (method)
    {
      case plCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return CompressZStd(uncompressedData, out_data);
#else
        plLog::Error("ZStd compression disabled in build settings!");
        return PLASMA_FAILURE;
#endif
      default:
        plLog::Error("Unsupported compression method {0}!", static_cast<plUInt32>(method));
        return PLASMA_FAILURE;
    }
  }

  plResult Decompress(plArrayPtr<const plUInt8> compressedData, plCompressionMethod method, plDynamicArray<plUInt8>& out_data)
  {
    out_data.Clear();

    if (compressedData.IsEmpty())
    {
      return PLASMA_SUCCESS;
    }

    switch (method)
    {
      case plCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return DecompressZStd(compressedData, out_data);
#else
        plLog::Error("ZStd compression disabled in build settings!");
        return PLASMA_FAILURE;
#endif
      default:
        plLog::Error("Unsupported compression method {0}!", static_cast<plUInt32>(method));
        return PLASMA_FAILURE;
    }
  }
} // namespace plCompressionUtils


PLASMA_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_Compression);
