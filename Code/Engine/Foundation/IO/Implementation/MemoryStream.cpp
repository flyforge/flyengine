#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>

// Reader implementation

plMemoryStreamReader::plMemoryStreamReader(const plMemoryStreamStorageInterface* pStreamStorage)
  : m_pStreamStorage(pStreamStorage)
{
}

plMemoryStreamReader::~plMemoryStreamReader() = default;

plUInt64 plMemoryStreamReader::ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead)
{
  PL_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const plUInt64 uiBytes = plMath::Min<plUInt64>(uiBytesToRead, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    plUInt64 uiBytesLeft = uiBytes;

    while (uiBytesLeft > 0)
    {
      plArrayPtr<const plUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiReadPosition);

      PL_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const plUInt64 toRead = plMath::Min<plUInt64>(data.GetCount(), uiBytesLeft);

      plMemoryUtils::Copy(static_cast<plUInt8*>(pReadBuffer), data.GetPtr(), static_cast<size_t>(toRead)); // Down-cast to size_t for 32-bit.

      pReadBuffer = plMemoryUtils::AddByteOffset(pReadBuffer, static_cast<size_t>(toRead)); // Down-cast to size_t for 32-bit.

      m_uiReadPosition += toRead;
      uiBytesLeft -= toRead;
    }
  }
  else
  {
    m_uiReadPosition += uiBytes;
  }

  return uiBytes;
}

plUInt64 plMemoryStreamReader::SkipBytes(plUInt64 uiBytesToSkip)
{
  PL_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  const plUInt64 uiBytes = plMath::Min<plUInt64>(uiBytesToSkip, m_pStreamStorage->GetStorageSize64() - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void plMemoryStreamReader::SetReadPosition(plUInt64 uiReadPosition)
{
  PL_ASSERT_RELEASE(uiReadPosition <= GetByteCount64(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

plUInt32 plMemoryStreamReader::GetByteCount32() const
{
  PL_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize32();
}

plUInt64 plMemoryStreamReader::GetByteCount64() const
{
  PL_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream reader needs a valid memory storage object!");

  return m_pStreamStorage->GetStorageSize64();
}

void plMemoryStreamReader::SetDebugSourceInformation(plStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////

// Writer implementation
plMemoryStreamWriter::plMemoryStreamWriter(plMemoryStreamStorageInterface* pStreamStorage)
  : m_pStreamStorage(pStreamStorage)

{
}

plMemoryStreamWriter::~plMemoryStreamWriter() = default;

plResult plMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  PL_ASSERT_DEV(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  if (uiBytesToWrite == 0)
    return PL_SUCCESS;

  PL_ASSERT_DEBUG(pWriteBuffer != nullptr, "No valid buffer containing data given!");

  // Reserve the memory in the storage object, grow size if appending data (don't shrink)
  m_pStreamStorage->SetInternalSize(plMath::Max(m_pStreamStorage->GetStorageSize64(), m_uiWritePosition + uiBytesToWrite));

  {
    plUInt64 uiBytesLeft = uiBytesToWrite;

    while (uiBytesLeft > 0)
    {
      plArrayPtr<plUInt8> data = m_pStreamStorage->GetContiguousMemoryRange(m_uiWritePosition);

      PL_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

      const plUInt64 toWrite = plMath::Min<plUInt64>(data.GetCount(), uiBytesLeft);

      plMemoryUtils::Copy(data.GetPtr(), static_cast<const plUInt8*>(pWriteBuffer), static_cast<size_t>(toWrite)); // Down-cast to size_t for 32-bit.

      pWriteBuffer = plMemoryUtils::AddByteOffset(pWriteBuffer, static_cast<size_t>(toWrite)); // Down-cast to size_t for 32-bit.

      m_uiWritePosition += toWrite;
      uiBytesLeft -= toWrite;
    }
  }

  return PL_SUCCESS;
}

void plMemoryStreamWriter::SetWritePosition(plUInt64 uiWritePosition)
{
  PL_ASSERT_RELEASE(m_pStreamStorage != nullptr, "The memory stream writer needs a valid memory storage object!");

  PL_ASSERT_RELEASE(uiWritePosition <= GetByteCount64(), "Write position must be between 0 and GetByteCount()!");
  m_uiWritePosition = uiWritePosition;
}

plUInt32 plMemoryStreamWriter::GetByteCount32() const
{
  PL_ASSERT_DEV(m_uiWritePosition <= 0xFFFFFFFFllu, "Use GetByteCount64 instead of GetByteCount32");
  return (plUInt32)m_uiWritePosition;
}

plUInt64 plMemoryStreamWriter::GetByteCount64() const
{
  return m_uiWritePosition;
}

//////////////////////////////////////////////////////////////////////////

plMemoryStreamStorageInterface::plMemoryStreamStorageInterface() = default;
plMemoryStreamStorageInterface::~plMemoryStreamStorageInterface() = default;

void plMemoryStreamStorageInterface::ReadAll(plStreamReader& inout_stream, plUInt64 uiMaxBytes /*= 0xFFFFFFFFFFFFFFFFllu*/)
{
  Clear();
  plMemoryStreamWriter w(this);

  plUInt8 uiTemp[1024 * 8];

  while (uiMaxBytes > 0)
  {
    const plUInt64 uiToRead = plMath::Min<plUInt64>(uiMaxBytes, PL_ARRAY_SIZE(uiTemp));

    const plUInt64 uiRead = inout_stream.ReadBytes(uiTemp, uiToRead);
    uiMaxBytes -= uiRead;

    w.WriteBytes(uiTemp, uiRead).IgnoreResult();

    if (uiRead < uiToRead)
      break;
  }
}

//////////////////////////////////////////////////////////////////////////


plRawMemoryStreamReader::plRawMemoryStreamReader() = default;

plRawMemoryStreamReader::plRawMemoryStreamReader(const void* pData, plUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

plRawMemoryStreamReader::~plRawMemoryStreamReader() = default;

void plRawMemoryStreamReader::Reset(const void* pData, plUInt64 uiDataSize)
{
  m_pRawMemory = static_cast<const plUInt8*>(pData);
  m_uiChunkSize = uiDataSize;
  m_uiReadPosition = 0;
}

plUInt64 plRawMemoryStreamReader::ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead)
{
  const plUInt64 uiBytes = plMath::Min<plUInt64>(uiBytesToRead, m_uiChunkSize - m_uiReadPosition);

  if (uiBytes == 0)
    return 0;

  if (pReadBuffer)
  {
    plMemoryUtils::Copy(static_cast<plUInt8*>(pReadBuffer), &m_pRawMemory[m_uiReadPosition], static_cast<size_t>(uiBytes));
  }

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

plUInt64 plRawMemoryStreamReader::SkipBytes(plUInt64 uiBytesToSkip)
{
  const plUInt64 uiBytes = plMath::Min<plUInt64>(uiBytesToSkip, m_uiChunkSize - m_uiReadPosition);

  m_uiReadPosition += uiBytes;

  return uiBytes;
}

void plRawMemoryStreamReader::SetReadPosition(plUInt64 uiReadPosition)
{
  PL_ASSERT_RELEASE(uiReadPosition < GetByteCount(), "Read position must be between 0 and GetByteCount()!");
  m_uiReadPosition = uiReadPosition;
}

plUInt64 plRawMemoryStreamReader::GetByteCount() const
{
  return m_uiChunkSize;
}

void plRawMemoryStreamReader::SetDebugSourceInformation(plStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////


plRawMemoryStreamWriter::plRawMemoryStreamWriter() = default;

plRawMemoryStreamWriter::plRawMemoryStreamWriter(void* pData, plUInt64 uiDataSize)
{
  Reset(pData, uiDataSize);
}

plRawMemoryStreamWriter::~plRawMemoryStreamWriter() = default;

void plRawMemoryStreamWriter::Reset(void* pData, plUInt64 uiDataSize)
{
  PL_ASSERT_DEV(pData != nullptr, "Invalid memory stream storage");

  m_pRawMemory = static_cast<plUInt8*>(pData);
  m_uiChunkSize = uiDataSize;
  m_uiWritePosition = 0;
}

plResult plRawMemoryStreamWriter::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  const plUInt64 uiBytes = plMath::Min<plUInt64>(uiBytesToWrite, m_uiChunkSize - m_uiWritePosition);

  plMemoryUtils::Copy(&m_pRawMemory[m_uiWritePosition], static_cast<const plUInt8*>(pWriteBuffer), static_cast<size_t>(uiBytes));

  m_uiWritePosition += uiBytes;

  if (uiBytes < uiBytesToWrite)
    return PL_FAILURE;

  return PL_SUCCESS;
}

plUInt64 plRawMemoryStreamWriter::GetStorageSize() const
{
  return m_uiChunkSize;
}

plUInt64 plRawMemoryStreamWriter::GetNumWrittenBytes() const
{
  return m_uiWritePosition;
}

void plRawMemoryStreamWriter::SetDebugSourceInformation(plStringView sDebugSourceInformation)
{
  m_sDebugSourceInformation = sDebugSourceInformation;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plDefaultMemoryStreamStorage::plDefaultMemoryStreamStorage(plUInt32 uiInitialCapacity, plAllocator* pAllocator)
  : m_Chunks(pAllocator)
{
  Reserve(uiInitialCapacity);
}

plDefaultMemoryStreamStorage::~plDefaultMemoryStreamStorage()
{
  Clear();
}

void plDefaultMemoryStreamStorage::Reserve(plUInt64 uiBytes)
{
  if (m_Chunks.IsEmpty())
  {
    auto& chunk = m_Chunks.ExpandAndGetRef();
    chunk.m_Bytes = plByteArrayPtr(m_InplaceMemory);
    chunk.m_uiStartOffset = 0;
    m_uiCapacity = m_Chunks[0].m_Bytes.GetCount();
  }

  while (m_uiCapacity < uiBytes)
  {
    AddChunk(static_cast<plUInt32>(plMath::Min<plUInt64>(uiBytes - m_uiCapacity, plMath::MaxValue<plUInt32>())));
  }
}

plUInt64 plDefaultMemoryStreamStorage::GetStorageSize64() const
{
  return m_uiInternalSize;
}

void plDefaultMemoryStreamStorage::Clear()
{
  m_uiInternalSize = 0;
  m_uiLastByteAccessed = 0;
  m_uiLastChunkAccessed = 0;
  Compact();
}

void plDefaultMemoryStreamStorage::Compact()
{
  // skip chunk 0, because that's where our inplace storage is used
  while (m_Chunks.GetCount() > 1)
  {
    auto& chunk = m_Chunks.PeekBack();

    if (m_uiInternalSize > m_uiCapacity - chunk.m_Bytes.GetCount())
      break;

    m_uiCapacity -= chunk.m_Bytes.GetCount();

    plUInt8* pData = chunk.m_Bytes.GetPtr();
    PL_DELETE_RAW_BUFFER(m_Chunks.GetAllocator(), pData);

    m_Chunks.PopBack();
  }
}

plUInt64 plDefaultMemoryStreamStorage::GetHeapMemoryUsage() const
{
  return m_Chunks.GetHeapMemoryUsage() + m_uiCapacity - m_Chunks[0].m_Bytes.GetCount();
}

plResult plDefaultMemoryStreamStorage::CopyToStream(plStreamWriter& inout_stream) const
{
  plUInt64 uiBytesLeft = m_uiInternalSize;
  plUInt64 uiReadPosition = 0;

  while (uiBytesLeft > 0)
  {
    plArrayPtr<const plUInt8> data = GetContiguousMemoryRange(uiReadPosition);

    PL_ASSERT_DEV(!data.IsEmpty(), "MemoryStreamStorage returned an empty contiguous memory block.");

    PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(data.GetPtr(), data.GetCount()));

    uiReadPosition += data.GetCount();
    uiBytesLeft -= data.GetCount();
  }

  return PL_SUCCESS;
}

plArrayPtr<const plUInt8> plDefaultMemoryStreamStorage::GetContiguousMemoryRange(plUInt64 uiStartByte) const
{
  if (uiStartByte >= m_uiInternalSize)
    return {};

  // remember the last access (byte offset) and in which chunk that ended up, to speed up this lookup
  // if a read comes in that's not AFTER the previous one, just reset to the start

  if (uiStartByte < m_uiLastByteAccessed)
  {
    m_uiLastChunkAccessed = 0;
  }

  m_uiLastByteAccessed = uiStartByte;

  for (; m_uiLastChunkAccessed < m_Chunks.GetCount(); ++m_uiLastChunkAccessed)
  {
    const auto& chunk = m_Chunks[m_uiLastChunkAccessed];

    if (uiStartByte < chunk.m_uiStartOffset + chunk.m_Bytes.GetCount())
    {
      const plUInt64 uiStartByteRel = uiStartByte - chunk.m_uiStartOffset;    // start offset into the chunk
      const plUInt64 uiMaxLenRel = chunk.m_Bytes.GetCount() - uiStartByteRel; // max number of bytes to use from this chunk
      const plUInt64 uiMaxRangeRel = m_uiInternalSize - uiStartByte;          // the 'stored data' might be less than the capacity of the chunk

      return {chunk.m_Bytes.GetPtr() + uiStartByteRel, static_cast<plUInt32>(plMath::Min<plUInt64>(uiMaxRangeRel, uiMaxLenRel))};
    }
  }

  return {};
}

plArrayPtr<plUInt8> plDefaultMemoryStreamStorage::GetContiguousMemoryRange(plUInt64 uiStartByte)
{
  plArrayPtr<const plUInt8> constData = const_cast<const plDefaultMemoryStreamStorage*>(this)->GetContiguousMemoryRange(uiStartByte);
  return {const_cast<plUInt8*>(constData.GetPtr()), constData.GetCount()};
}

void plDefaultMemoryStreamStorage::SetInternalSize(plUInt64 uiSize)
{
  Reserve(uiSize);

  m_uiInternalSize = uiSize;
}

void plDefaultMemoryStreamStorage::AddChunk(plUInt32 uiMinimumSize)
{
  auto& chunk = m_Chunks.ExpandAndGetRef();

  plUInt32 uiSize = 0;

  if (m_Chunks.GetCount() < 4)
  {
    uiSize = 1024 * 4; // 4 KB
  }
  else if (m_Chunks.GetCount() < 8)
  {
    uiSize = 1024 * 64; // 64 KB
  }
  else if (m_Chunks.GetCount() < 16)
  {
    uiSize = 1024 * 1024 * 4; // 4 MB
  }
  else
  {
    uiSize = 1024 * 1024 * 64; // 64 MB
  }

  uiSize = plMath::Max(uiSize, uiMinimumSize);

  const auto& prevChunk = m_Chunks[m_Chunks.GetCount() - 2];

  chunk.m_Bytes = plArrayPtr<plUInt8>(PL_NEW_RAW_BUFFER(m_Chunks.GetAllocator(), plUInt8, uiSize), uiSize);
  chunk.m_uiStartOffset = prevChunk.m_uiStartOffset + prevChunk.m_Bytes.GetCount();
  m_uiCapacity += chunk.m_Bytes.GetCount();
}


