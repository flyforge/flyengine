#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/FileWriter.h>

plResult plFileWriter::Open(plStringView sFile, plUInt32 uiCacheSize /*= 1024 * 1024*/, plFileShareMode::Enum fileShareMode /*= plFileShareMode::Exclusive*/, bool bAllowFileEvents /*= true*/)
{
  uiCacheSize = plMath::Clamp<plUInt32>(uiCacheSize, 1024, 1024 * 1024 * 32);

  m_pDataDirWriter = GetFileWriter(sFile, fileShareMode, bAllowFileEvents);

  if (!m_pDataDirWriter)
    return PLASMA_FAILURE;

  m_Cache.SetCountUninitialized(uiCacheSize);

  m_uiCacheWritePosition = 0;

  return PLASMA_SUCCESS;
}

void plFileWriter::Close()
{
  if (!m_pDataDirWriter)
    return;

  Flush().IgnoreResult();

  m_pDataDirWriter->Close();
  m_pDataDirWriter = nullptr;
}

plResult plFileWriter::Flush()
{
  const plResult res = m_pDataDirWriter->Write(&m_Cache[0], m_uiCacheWritePosition);
  m_uiCacheWritePosition = 0;

  return res;
}

plResult plFileWriter::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  PLASMA_ASSERT_DEV(m_pDataDirWriter != nullptr, "The file has not been opened (successfully).");

  if (uiBytesToWrite > m_Cache.GetCount())
  {
    // if there is more incoming data than what our cache can hold, there is no point in storing a copy
    // instead we can just pass the entire data through right away

    if (m_uiCacheWritePosition > 0)
    {
      PLASMA_SUCCEED_OR_RETURN(Flush());
    }

    return m_pDataDirWriter->Write(pWriteBuffer, uiBytesToWrite);
  }
  else
  {
    plUInt8* pBuffer = (plUInt8*)pWriteBuffer;

    while (uiBytesToWrite > 0)
    {
      // determine chunk size to be written
      plUInt64 uiChunkSize = uiBytesToWrite;

      const plUInt64 uiRemainingCache = m_Cache.GetCount() - m_uiCacheWritePosition;

      if (uiRemainingCache < uiBytesToWrite)
        uiChunkSize = uiRemainingCache;

      // copy memory
      plMemoryUtils::Copy(&m_Cache[(plUInt32)m_uiCacheWritePosition], pBuffer, (plUInt32)uiChunkSize);

      pBuffer += uiChunkSize;
      m_uiCacheWritePosition += uiChunkSize;
      uiBytesToWrite -= uiChunkSize;

      // if the cache is full or nearly full, flush it to disk
      if (m_uiCacheWritePosition + 32 >= m_Cache.GetCount())
      {
        if (Flush() == PLASMA_FAILURE)
          return PLASMA_FAILURE;
      }
    }

    return PLASMA_SUCCESS;
  }
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileWriter);
