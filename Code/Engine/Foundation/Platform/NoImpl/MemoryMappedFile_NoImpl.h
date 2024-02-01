#include <Foundation/FoundationPCH.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>

struct plMemoryMappedFileImpl
{
  plMemoryMappedFile::Mode m_Mode = plMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  plUInt64 m_uiFileSize = 0;

  ~plMemoryMappedFileImpl() {}
};

plMemoryMappedFile::plMemoryMappedFile()
{
  m_pImpl = PL_DEFAULT_NEW(plMemoryMappedFileImpl);
}

plMemoryMappedFile::~plMemoryMappedFile()
{
  Close();
}

void plMemoryMappedFile::Close()
{
  m_pImpl = PL_DEFAULT_NEW(plMemoryMappedFileImpl);
}

plMemoryMappedFile::Mode plMemoryMappedFile::GetMode() const
{
  return m_pImpl->m_Mode;
}

const void* plMemoryMappedFile::GetReadPointer(plUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/) const
{
  PL_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  return m_pImpl->m_pMappedFilePtr;
}

void* plMemoryMappedFile::GetWritePointer(plUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  PL_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  return m_pImpl->m_pMappedFilePtr;
}

plUInt64 plMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}
