#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/CompressedStreamBrotliG.h>

#ifdef BUILDSYSTEM_ENABLE_BROTLIG_SUPPORT

#  include <Foundation/System/SystemInformation.h>
#  include <brotlig/inc/BrotliG.h>

plCompressedStreamReaderBrotliG::plCompressedStreamReaderBrotliG() = default;

plCompressedStreamReaderBrotliG::plCompressedStreamReaderBrotliG(plStreamReader* pInputStream)
{
  SetInputStream(pInputStream);
}

plCompressedStreamReaderBrotliG::~plCompressedStreamReaderBrotliG()
{
}

void plCompressedStreamReaderBrotliG::SetInputStream(plStreamReader* pInputStream)
{
  m_bReachedEnd = false;
  m_pInputStream = pInputStream;

  if (pInputStream != nullptr)
  {
    plUInt32 uiCompressedSize = 0;

    pInputStream->ReadDWordValue(&uiCompressedSize).AssertSuccess();
    m_CompressedCache.SetCountUninitialized(uiCompressedSize);

    pInputStream->ReadBytes(m_CompressedCache.GetData(), uiCompressedSize);

    plUInt16 uiTerminator = 0;
    pInputStream->ReadBytes(&uiTerminator, sizeof(plUInt16));
    PLASMA_ASSERT_DEV(uiTerminator == 0, "Invalid Brotli stream");

    plUInt32 uiDecompressedSize = BrotliG::DecompressedSize(m_CompressedCache.GetData());
    m_DecompressedData.SetCountUninitialized(uiDecompressedSize);

    const auto res = BrotliG::DecodeCPU(uiCompressedSize, m_CompressedCache.GetData(), &uiDecompressedSize, m_DecompressedData.GetData(), nullptr);
    PLASMA_ASSERT_DEV(res == BROTLIG_OK, "Decompression failed");

    m_DecompressedData.SetCount(uiDecompressedSize);
    m_CompressedCache.Clear();
  }
}

plUInt64 plCompressedStreamReaderBrotliG::ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead)
{
  PLASMA_ASSERT_DEV(m_pInputStream != nullptr, "No input stream has been specified");

  if (uiBytesToRead == 0 || m_bReachedEnd)
    return 0;

  const plUInt64 uiReadSize = plMath::Min(uiBytesToRead, m_DecompressedData.GetCount() - m_uiCursor);

  if (uiReadSize == 0)
    return 0;

  // Implement the 'skip n bytes' feature
  if (pReadBuffer == nullptr)
  {
    m_uiCursor += uiReadSize;
    return uiReadSize;
  }

  plUInt64 uiBytesRead = 0;
  while (uiBytesRead < uiReadSize)
  {
    static_cast<plUInt8*>(pReadBuffer)[uiBytesRead] = m_DecompressedData[static_cast<plUInt32>(m_uiCursor + uiBytesRead)];
    uiBytesRead++;
  }

  m_uiCursor += uiReadSize;

  if (m_uiCursor == m_DecompressedData.GetCount())
    m_bReachedEnd = true;

  return uiReadSize;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plCompressedStreamWriterBrotliG::plCompressedStreamWriterBrotliG() = default;

plCompressedStreamWriterBrotliG::plCompressedStreamWriterBrotliG(plStreamWriter* pOutputStream)
{
  SetOutputStream(pOutputStream);
}

plCompressedStreamWriterBrotliG::~plCompressedStreamWriterBrotliG()
{
  if (m_pOutputStream != nullptr)
  {
    // NOTE: FinishCompressedStream() WILL write a couple of bytes, even if the user did not write anything.
    // If plCompressedStreamWriterBrotliG was not supposed to be used, this may end up in a corrupted output file.
    // PLASMA_ASSERT_DEV(m_uiWrittenBytes > 0, "Output stream was set, but not a single byte was written to the compressed stream before destruction.
    // Incorrect usage?");

    FinishCompressedStream().IgnoreResult();
  }
}

void plCompressedStreamWriterBrotliG::SetOutputStream(plStreamWriter* pOutputStream, plUInt32 uiPageSizeKB)
{
  if (m_pOutputStream == pOutputStream)
    return;

  // limit the cache to 63KB, because at 64KB we run into an endless loop due to a 16 bit overflow
  uiPageSizeKB = plMath::Max(uiPageSizeKB, 1u);

  // finish anything done on a previous output stream
  FinishCompressedStream().IgnoreResult();

  m_UncompressedCache.Clear();
  m_CompressedCache.Clear();

  m_uiUncompressedSize = 0;
  m_uiCompressedSize = 0;
  m_uiWrittenBytes = 0;
  m_uiPageSize = static_cast<plUInt64>(uiPageSizeKB) * 1024;

  if (pOutputStream != nullptr)
  {
    m_pOutputStream = pOutputStream;
  }
}

plResult plCompressedStreamWriterBrotliG::FinishCompressedStream()
{
  if (m_pOutputStream == nullptr)
    return PLASMA_SUCCESS;

  if (Flush().Failed())
    return PLASMA_FAILURE;

  // write a zero-terminator
  constexpr plUInt16 uiTerminator = 0;
  PLASMA_SUCCEED_OR_RETURN(m_pOutputStream->WriteBytes(&uiTerminator, sizeof(plUInt16)));

  m_uiWrittenBytes += sizeof(plUInt16);
  m_pOutputStream = nullptr;

  return PLASMA_SUCCESS;
}

plResult plCompressedStreamWriterBrotliG::Flush()
{
  if (m_pOutputStream == nullptr)
    return PLASMA_SUCCESS;

  const plUInt32 uiMaxCompressedSize = BrotliG::MaxCompressedSize(static_cast<plUInt32>(m_uiUncompressedSize));
  m_CompressedCache.SetCountUninitialized(plMath::Max(1U, uiMaxCompressedSize));

  plUInt32 uiCompressedSize = 0;
  if (BrotliG::Encode(static_cast<plUInt32>(m_uiUncompressedSize), m_UncompressedCache.GetData(), &uiCompressedSize, m_CompressedCache.GetData(), nullptr) != BROTLIG_OK)
    return PLASMA_FAILURE;

  PLASMA_SUCCEED_OR_RETURN(m_pOutputStream->WriteDWordValue(&uiCompressedSize));
  PLASMA_SUCCEED_OR_RETURN(m_pOutputStream->WriteBytes(m_CompressedCache.GetData(), uiCompressedSize));

  m_UncompressedCache.Clear();
  m_CompressedCache.Clear();

  m_uiCompressedSize = uiCompressedSize;

  return PLASMA_SUCCESS;
}

plResult plCompressedStreamWriterBrotliG::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  m_uiUncompressedSize += uiBytesToWrite;
  m_UncompressedCache.Reserve(static_cast<plUInt32>(m_uiUncompressedSize));

  const auto* pData = static_cast<const plUInt8*>(pWriteBuffer);
  m_UncompressedCache.PushBackRange(plMakeArrayPtr(pData, static_cast<plUInt32>(uiBytesToWrite)));

  return PLASMA_SUCCESS;
}

#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_CompressedStreamBrotliG);
