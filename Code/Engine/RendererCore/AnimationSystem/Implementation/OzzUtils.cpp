#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <RendererCore/AnimationSystem/Implementation/OzzUtils.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/io/archive.h>

plOzzArchiveData::plOzzArchiveData() = default;
plOzzArchiveData::~plOzzArchiveData() = default;

plResult plOzzArchiveData::FetchRegularFile(const char* szFile)
{
  plFileReader file;
  PL_SUCCEED_OR_RETURN(file.Open(szFile));

  m_Storage.Clear();
  m_Storage.Reserve(file.GetFileSize());
  m_Storage.ReadAll(file);

  return PL_SUCCESS;
}

plResult plOzzArchiveData::FetchEmbeddedArchive(plStreamReader& inout_stream)
{
  char szTag[8] = "";

  inout_stream.ReadBytes(szTag, 8);
  szTag[7] = '\0';

  if (!plStringUtils::IsEqual(szTag, "plOzzAr"))
    return PL_FAILURE;

  /*const plTypeVersion version =*/inout_stream.ReadVersion(1);

  plUInt64 uiArchiveSize = 0;
  inout_stream >> uiArchiveSize;

  m_Storage.Clear();
  m_Storage.Reserve(uiArchiveSize);
  m_Storage.ReadAll(inout_stream, uiArchiveSize);

  if (m_Storage.GetStorageSize64() != uiArchiveSize)
    return PL_FAILURE;

  return PL_SUCCESS;
}

plResult plOzzArchiveData::StoreEmbeddedArchive(plStreamWriter& inout_stream) const
{
  const char szTag[8] = "plOzzAr";

  PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(szTag, 8));

  inout_stream.WriteVersion(1);

  const plUInt64 uiArchiveSize = m_Storage.GetStorageSize64();

  inout_stream << uiArchiveSize;

  return m_Storage.CopyToStream(inout_stream);
}

plOzzStreamReader::plOzzStreamReader(const plOzzArchiveData& data)
  : m_Reader(&data.m_Storage)
{
}

bool plOzzStreamReader::opened() const
{
  return true;
}

size_t plOzzStreamReader::Read(void* pBuffer, size_t uiSize)
{
  return static_cast<size_t>(m_Reader.ReadBytes(pBuffer, uiSize));
}

size_t plOzzStreamReader::Write(const void* pBuffer, size_t uiSize)
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

int plOzzStreamReader::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Reader.SetReadPosition(m_Reader.GetReadPosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Reader.SetReadPosition(m_Reader.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Reader.SetReadPosition(iOffset);
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int plOzzStreamReader::Tell() const
{
  return static_cast<int>(m_Reader.GetReadPosition());
}

size_t plOzzStreamReader::Size() const
{
  return static_cast<size_t>(m_Reader.GetByteCount64());
}

plOzzStreamWriter::plOzzStreamWriter(plOzzArchiveData& ref_data)
  : m_Writer(&ref_data.m_Storage)
{
}

bool plOzzStreamWriter::opened() const
{
  return true;
}

size_t plOzzStreamWriter::Read(void* pBuffer, size_t uiSize)
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return 0;
}

size_t plOzzStreamWriter::Write(const void* pBuffer, size_t uiSize)
{
  if (m_Writer.WriteBytes(pBuffer, uiSize).Failed())
    return 0;

  return uiSize;
}

int plOzzStreamWriter::Seek(int iOffset, Origin origin)
{
  switch (origin)
  {
    case ozz::io::Stream::kCurrent:
      m_Writer.SetWritePosition(m_Writer.GetWritePosition() + iOffset);
      break;
    case ozz::io::Stream::kEnd:
      m_Writer.SetWritePosition(m_Writer.GetByteCount64() - iOffset);
      break;
    case ozz::io::Stream::kSet:
      m_Writer.SetWritePosition(iOffset);
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0;
}

int plOzzStreamWriter::Tell() const
{
  return static_cast<int>(m_Writer.GetWritePosition());
}

size_t plOzzStreamWriter::Size() const
{
  return static_cast<size_t>(m_Writer.GetByteCount64());
}

void plOzzUtils::CopyAnimation(ozz::animation::Animation* pDst, const ozz::animation::Animation* pSrc)
{
  plOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    plOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    plOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}

PL_RENDERERCORE_DLL void plOzzUtils::CopySkeleton(ozz::animation::Skeleton* pDst, const ozz::animation::Skeleton* pSrc)
{
  plOzzArchiveData ozzArchiveData;

  // store in ozz archive
  {
    plOzzStreamWriter ozzWriter(ozzArchiveData);
    ozz::io::OArchive ozzArchive(&ozzWriter);

    ozzArchive << *pSrc;
  }

  // read it from archive again
  {
    plOzzStreamReader ozzReader(ozzArchiveData);
    ozz::io::IArchive ozzArchive(&ozzReader);

    ozzArchive >> *pDst;
  }
}


