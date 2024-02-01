#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/CompressedStreamZstd.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

#  include <Foundation/System/SystemInformation.h>
#  include <zstd/zstd.h>

plCompressedStreamReaderZstd::plCompressedStreamReaderZstd() = default;

plCompressedStreamReaderZstd::plCompressedStreamReaderZstd(plStreamReader* pInputStream)
{
  SetInputStream(pInputStream);
}

plCompressedStreamReaderZstd::~plCompressedStreamReaderZstd()
{
  if (m_pZstdDStream != nullptr)
  {
    ZSTD_freeDStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream));
    m_pZstdDStream = nullptr;
  }
}

void plCompressedStreamReaderZstd::SetInputStream(plStreamReader* pInputStream)
{
  m_InBuffer.pos = 0;
  m_InBuffer.size = 0;
  m_bReachedEnd = false;
  m_pInputStream = pInputStream;

  if (m_pZstdDStream == nullptr)
  {
    m_pZstdDStream = ZSTD_createDStream();
  }

  ZSTD_initDStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream));
}

plUInt64 plCompressedStreamReaderZstd::ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead)
{
  PL_ASSERT_DEV(m_pInputStream != nullptr, "No input stream has been specified");

  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  // Implement the 'skip n bytes' feature with a temp cache
  if (pReadBuffer == nullptr)
  {
    plUInt64 uiBytesRead = 0;
    plUInt8 uiTemp[1024];

    while (uiBytesToRead > 0)
    {
      const plUInt32 uiToRead = plMath::Min<plUInt32>(static_cast<plUInt32>(uiBytesToRead), 1024);

      const plUInt64 uiGotBytes = ReadBytes(uiTemp, uiToRead);

      uiBytesRead += uiGotBytes;
      uiBytesToRead -= uiGotBytes;

      if (uiGotBytes == 0) // prevent an endless loop
        break;
    }

    return uiBytesRead;
  }

  ZSTD_outBuffer outBuffer;
  outBuffer.dst = pReadBuffer;
  outBuffer.pos = 0;
  outBuffer.size = plMath::SafeConvertToSizeT(uiBytesToRead);

  while (outBuffer.pos < outBuffer.size)
  {
    if (RefillReadCache().Failed())
      return outBuffer.pos;

    const size_t res = ZSTD_decompressStream(reinterpret_cast<ZSTD_DStream*>(m_pZstdDStream), &outBuffer, reinterpret_cast<ZSTD_inBuffer*>(&m_InBuffer));
    PL_IGNORE_UNUSED(res);
    PL_ASSERT_DEV(!ZSTD_isError(res), "Decompressing the stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  if (m_InBuffer.pos == m_InBuffer.size)
  {
    // if we have reached the end, we have not yet read the zero-terminator
    // do this now, so that data that comes after the compressed stream can be read properly

    RefillReadCache().IgnoreResult();
  }

  return outBuffer.pos;
}

plResult plCompressedStreamReaderZstd::RefillReadCache()
{
  // if our input buffer is empty, we need to read more into our cache
  if (m_InBuffer.pos == m_InBuffer.size)
  {
    plUInt16 uiCompressedSize = 0;
    PL_VERIFY(m_pInputStream->ReadBytes(&uiCompressedSize, sizeof(plUInt16)) == sizeof(plUInt16), "Reading the compressed chunk size from the input stream failed.");

    m_InBuffer.pos = 0;
    m_InBuffer.size = uiCompressedSize;

    if (uiCompressedSize > 0)
    {
      if (m_CompressedCache.GetCount() < uiCompressedSize)
      {
        m_CompressedCache.SetCountUninitialized(plMath::RoundUp(uiCompressedSize, 1024));

        m_InBuffer.src = m_CompressedCache.GetData();
      }

      PL_VERIFY(m_pInputStream->ReadBytes(m_CompressedCache.GetData(), sizeof(plUInt8) * uiCompressedSize) == sizeof(plUInt8) * uiCompressedSize, "Reading the compressed chunk of size {0} from the input stream failed.", uiCompressedSize);
    }
  }

  // if the input buffer is still empty, there was no more data to read (we reached the zero-terminator)
  if (m_InBuffer.size == 0)
  {
    // in this case there is also no output that can be generated anymore
    m_bReachedEnd = true;
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCompressedStreamWriterZstd::plCompressedStreamWriterZstd() = default;

plCompressedStreamWriterZstd::plCompressedStreamWriterZstd(plStreamWriter* pOutputStream, plUInt32 uiMaxNumWorkerThreads, Compression ratio /*= Compression::Default*/, plUInt32 uiCompressionCacheSizeKB /*= 4*/)
{
  SetOutputStream(pOutputStream, uiMaxNumWorkerThreads, ratio, uiCompressionCacheSizeKB);
}

plCompressedStreamWriterZstd::~plCompressedStreamWriterZstd()
{
  if (m_pOutputStream != nullptr)
  {
    // NOTE: FinishCompressedStream() WILL write a couple of bytes, even if the user did not write anything.
    // If plCompressedStreamWriterZstd was not supposed to be used, this may end up in a corrupted output file.
    // PL_ASSERT_DEV(m_uiWrittenBytes > 0, "Output stream was set, but not a single byte was written to the compressed stream before destruction.
    // Incorrect usage?");

    FinishCompressedStream().IgnoreResult();
  }

  if (m_pZstdCStream)
  {
    ZSTD_freeCStream(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream));
    m_pZstdCStream = nullptr;
  }
}

void plCompressedStreamWriterZstd::SetOutputStream(plStreamWriter* pOutputStream, plUInt32 uiMaxNumWorkerThreads, Compression ratio /*= Compression::Default*/, plUInt32 uiCompressionCacheSizeKB /*= 4*/)
{
  if (m_pOutputStream == pOutputStream)
    return;

  // limit the cache to 63KB, because at 64KB we run into an endless loop due to a 16 bit overflow
  uiCompressionCacheSizeKB = plMath::Min(uiCompressionCacheSizeKB, 63u);

  // finish anything done on a previous output stream
  FinishCompressedStream().IgnoreResult();

  m_uiUncompressedSize = 0;
  m_uiCompressedSize = 0;
  m_uiWrittenBytes = 0;

  if (pOutputStream != nullptr)
  {
    m_pOutputStream = pOutputStream;

    if (m_pZstdCStream == nullptr)
    {
      m_pZstdCStream = ZSTD_createCStream();
    }

    const plUInt32 uiCoreCount = (uiMaxNumWorkerThreads > 0) ? plMath::Clamp(plSystemInformation::Get().GetCPUCoreCount(), 1u, uiMaxNumWorkerThreads) : 0u;

    ZSTD_CCtx_reset(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_reset_session_only);
    ZSTD_CCtx_refCDict(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), nullptr);
    ZSTD_CCtx_setParameter(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_c_compressionLevel, (int)ratio);
    ZSTD_CCtx_setParameter(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), ZSTD_c_nbWorkers, uiCoreCount);

    m_CompressedCache.SetCountUninitialized(plMath::Max(1U, uiCompressionCacheSizeKB) * 1024);

    m_OutBuffer.dst = m_CompressedCache.GetData();
    m_OutBuffer.pos = 0;
    m_OutBuffer.size = m_CompressedCache.GetCount();
  }
}

plResult plCompressedStreamWriterZstd::FinishCompressedStream()
{
  if (m_pOutputStream == nullptr)
    return PL_SUCCESS;

  if (Flush().Failed())
    return PL_FAILURE;

  ZSTD_inBuffer emptyBuffer;
  emptyBuffer.pos = 0;
  emptyBuffer.size = 0;
  emptyBuffer.src = nullptr;

  const size_t res = ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &emptyBuffer, ZSTD_e_end);
  PL_VERIFY(!ZSTD_isError(res), "Deinitializing the zstd compression stream failed: '{0}'", ZSTD_getErrorName(res));

  // one more flush to write out the last chunk
  if (FlushWriteCache() == PL_FAILURE)
    return PL_FAILURE;

  // write a zero-terminator
  const plUInt16 uiTerminator = 0;
  if (m_pOutputStream->WriteBytes(&uiTerminator, sizeof(plUInt16)) == PL_FAILURE)
    return PL_FAILURE;

  m_uiWrittenBytes += sizeof(plUInt16);
  m_pOutputStream = nullptr;

  return PL_SUCCESS;
}

plResult plCompressedStreamWriterZstd::Flush()
{
  if (m_pOutputStream == nullptr)
    return PL_SUCCESS;

  ZSTD_inBuffer emptyBuffer;
  emptyBuffer.pos = 0;
  emptyBuffer.size = 0;
  emptyBuffer.src = nullptr;

  while (ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &emptyBuffer, ZSTD_e_flush) > 0)
  {
    if (FlushWriteCache() == PL_FAILURE)
      return PL_FAILURE;
  }

  if (FlushWriteCache() == PL_FAILURE)
    return PL_FAILURE;

  return PL_SUCCESS;
}

plResult plCompressedStreamWriterZstd::FlushWriteCache()
{
  if (m_pOutputStream == nullptr)
    return PL_SUCCESS;

  const plUInt16 uiUsedCache = static_cast<plUInt16>(m_OutBuffer.pos);

  if (uiUsedCache == 0)
    return PL_SUCCESS;

  if (m_pOutputStream->WriteBytes(&uiUsedCache, sizeof(plUInt16)) == PL_FAILURE)
    return PL_FAILURE;

  if (m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), sizeof(plUInt8) * uiUsedCache) == PL_FAILURE)
    return PL_FAILURE;

  m_uiCompressedSize += uiUsedCache;
  m_uiWrittenBytes += sizeof(plUInt16) + uiUsedCache;

  // reset the write position
  m_OutBuffer.pos = 0;

  return PL_SUCCESS;
}

plResult plCompressedStreamWriterZstd::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  PL_ASSERT_DEV(m_pZstdCStream != nullptr, "The stream is already closed, you cannot write more data to it.");

  m_uiUncompressedSize += static_cast<plUInt32>(uiBytesToWrite);

  ZSTD_inBuffer inBuffer;
  inBuffer.pos = 0;
  inBuffer.src = pWriteBuffer;
  inBuffer.size = static_cast<size_t>(uiBytesToWrite);

  while (inBuffer.pos < inBuffer.size)
  {
    if (m_OutBuffer.pos == m_OutBuffer.size)
    {
      if (FlushWriteCache() == PL_FAILURE)
        return PL_FAILURE;
    }

    const size_t res = ZSTD_compressStream2(reinterpret_cast<ZSTD_CStream*>(m_pZstdCStream), reinterpret_cast<ZSTD_outBuffer*>(&m_OutBuffer), &inBuffer, ZSTD_e_continue);

    PL_VERIFY(!ZSTD_isError(res), "Compressing the zstd stream failed: '{0}'", ZSTD_getErrorName(res));
  }

  return PL_SUCCESS;
}

#endif


