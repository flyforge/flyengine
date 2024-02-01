#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/ChunkStream.h>

plChunkStreamWriter::plChunkStreamWriter(plStreamWriter& inout_stream)
  : m_Stream(inout_stream)
{
  m_bWritingFile = false;
  m_bWritingChunk = false;
}

void plChunkStreamWriter::BeginStream(plUInt16 uiVersion)
{
  PL_ASSERT_DEV(!m_bWritingFile, "Already writing the file.");
  PL_ASSERT_DEV(uiVersion > 0, "The version number must be larger than 0");

  m_bWritingFile = true;

  const char* szTag = "BGNCHNK2";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
  m_Stream.WriteBytes(&uiVersion, 2).IgnoreResult();
}

void plChunkStreamWriter::EndStream()
{
  PL_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  PL_ASSERT_DEV(!m_bWritingChunk, "A chunk is still open for writing: '{0}'", m_sChunkName);

  m_bWritingFile = false;

  const char* szTag = "END CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();
}

void plChunkStreamWriter::BeginChunk(plStringView sName, plUInt32 uiVersion)
{
  PL_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  PL_ASSERT_DEV(!m_bWritingChunk, "A chunk is already open for writing: '{0}'", m_sChunkName);

  m_sChunkName = sName;

  const char* szTag = "NXT CHNK";
  m_Stream.WriteBytes(szTag, 8).IgnoreResult();

  m_Stream << m_sChunkName;
  m_Stream << uiVersion;

  m_bWritingChunk = true;
}


void plChunkStreamWriter::EndChunk()
{
  PL_ASSERT_DEV(m_bWritingFile, "Not writing to the file.");
  PL_ASSERT_DEV(m_bWritingChunk, "No chunk is currently open.");

  m_bWritingChunk = false;

  const plUInt32 uiStorageSize = m_Storage.GetCount();
  m_Stream << uiStorageSize;
  /// \todo Write Chunk CRC

  for (plUInt32 i = 0; i < uiStorageSize;)
  {
    const plUInt32 uiRange = m_Storage.GetContiguousRange(i);

    PL_ASSERT_DEBUG(uiRange > 0, "Invalid contiguous range");

    m_Stream.WriteBytes(&m_Storage[i], uiRange).IgnoreResult();
    i += uiRange;
  }

  m_Storage.Clear();
}

plResult plChunkStreamWriter::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  PL_ASSERT_DEV(m_bWritingChunk, "No chunk is currently written to");

  const plUInt8* pBytes = (const plUInt8*)pWriteBuffer;

  for (plUInt64 i = 0; i < uiBytesToWrite; ++i)
    m_Storage.PushBack(pBytes[i]);

  return PL_SUCCESS;
}



plChunkStreamReader::plChunkStreamReader(plStreamReader& inout_stream)
  : m_Stream(inout_stream)
{
  m_ChunkInfo.m_bValid = false;
  m_EndChunkFileMode = EndChunkFileMode::JustClose;
}

plUInt64 plChunkStreamReader::ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead)
{
  PL_ASSERT_DEV(m_ChunkInfo.m_bValid, "No valid chunk available.");

  uiBytesToRead = plMath::Min<plUInt64>(uiBytesToRead, m_ChunkInfo.m_uiUnreadChunkBytes);
  m_ChunkInfo.m_uiUnreadChunkBytes -= (plUInt32)uiBytesToRead;

  return m_Stream.ReadBytes(pReadBuffer, uiBytesToRead);
}

plUInt16 plChunkStreamReader::BeginStream()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  plUInt16 uiVersion = 0;

  if (plStringUtils::IsEqual(szTag, "BGNCHNK2"))
  {
    m_Stream.ReadBytes(&uiVersion, 2);
  }
  else
  {
    // "BGN CHNK" is the old chunk identifier, before a version number was written
    PL_ASSERT_DEV(plStringUtils::IsEqual(szTag, "BGN CHNK"), "Not a valid chunk file.");
  }

  TryReadChunkHeader();
  return uiVersion;
}

void plChunkStreamReader::EndStream()
{
  if (m_EndChunkFileMode == EndChunkFileMode::SkipToEnd)
  {
    while (m_ChunkInfo.m_bValid)
      NextChunk();
  }
}

void plChunkStreamReader::TryReadChunkHeader()
{
  m_ChunkInfo.m_bValid = false;

  char szTag[9];
  m_Stream.ReadBytes(szTag, 8);
  szTag[8] = '\0';

  if (plStringUtils::IsEqual(szTag, "END CHNK"))
    return;

  if (plStringUtils::IsEqual(szTag, "NXT CHNK"))
  {
    m_Stream >> m_ChunkInfo.m_sChunkName;
    m_Stream >> m_ChunkInfo.m_uiChunkVersion;
    m_Stream >> m_ChunkInfo.m_uiChunkBytes;
    m_ChunkInfo.m_uiUnreadChunkBytes = m_ChunkInfo.m_uiChunkBytes;

    m_ChunkInfo.m_bValid = true;

    return;
  }

  PL_REPORT_FAILURE("Invalid chunk file, tag is '{0}'", szTag);
}

void plChunkStreamReader::NextChunk()
{
  if (!m_ChunkInfo.m_bValid)
    return;

  const plUInt64 uiToSkip = m_ChunkInfo.m_uiUnreadChunkBytes;
  const plUInt64 uiSkipped = SkipBytes(uiToSkip);
  PL_VERIFY(uiSkipped == uiToSkip, "Corrupt chunk '{0}' (version {1}), tried to skip {2} bytes, could only read {3} bytes", m_ChunkInfo.m_sChunkName, m_ChunkInfo.m_uiChunkVersion, uiToSkip, uiSkipped);

  TryReadChunkHeader();
}


