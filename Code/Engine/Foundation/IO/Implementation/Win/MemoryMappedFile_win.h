#include <Foundation/IO/Implementation/Win/DosDevicePath_win.h>
#include <Foundation/IO/MemoryMappedFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/StringConversion.h>

struct plMemoryMappedFileImpl
{
  plMemoryMappedFile::Mode m_Mode = plMemoryMappedFile::Mode::None;
  void* m_pMappedFilePtr = nullptr;
  plUInt64 m_uiFileSize = 0;
  HANDLE m_hFile = INVALID_HANDLE_VALUE;
  HANDLE m_hMapping = INVALID_HANDLE_VALUE;

  ~plMemoryMappedFileImpl()
  {
    if (m_pMappedFilePtr != nullptr)
    {
      UnmapViewOfFile(m_pMappedFilePtr);
      m_pMappedFilePtr = nullptr;
    }

    if (m_hMapping != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hMapping);
      m_hMapping = INVALID_HANDLE_VALUE;
    }

    if (m_hFile != INVALID_HANDLE_VALUE)
    {
      CloseHandle(m_hFile);
      m_hFile = INVALID_HANDLE_VALUE;
    }
  }
};

plMemoryMappedFile::plMemoryMappedFile()
{
  m_pImpl = PLASMA_DEFAULT_NEW(plMemoryMappedFileImpl);
}

plMemoryMappedFile::~plMemoryMappedFile()
{
  Close();
}

plResult plMemoryMappedFile::Open(plStringView sAbsolutePath, Mode mode)
{
  PLASMA_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  PLASMA_ASSERT_DEV(plPathUtils::IsAbsolutePath(sAbsolutePath), "plMemoryMappedFile::Open() can only be used with absolute file paths");

  PLASMA_LOG_BLOCK("MemoryMapFile", sAbsolutePath);


  Close();

  m_pImpl->m_Mode = mode;

  DWORD access = GENERIC_READ;

  if (mode == Mode::ReadWrite)
  {
    access |= GENERIC_WRITE;
  }

  m_pImpl->m_hFile = CreateFileW(plDosDevicePath(sAbsolutePath), access, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

  DWORD errorCode = GetLastError();

  if (m_pImpl->m_hFile == nullptr || m_pImpl->m_hFile == INVALID_HANDLE_VALUE)
  {
    plLog::Error("Could not open file for memory mapping - {}", plArgErrorCode(errorCode));
    Close();
    return PLASMA_FAILURE;
  }

  if (GetFileSizeEx(m_pImpl->m_hFile, reinterpret_cast<LARGE_INTEGER*>(&m_pImpl->m_uiFileSize)) == FALSE || m_pImpl->m_uiFileSize == 0)
  {
    plLog::Error("File for memory mapping is empty");
    Close();
    return PLASMA_FAILURE;
  }

  m_pImpl->m_hMapping = CreateFileMappingW(m_pImpl->m_hFile, nullptr, m_pImpl->m_Mode == Mode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, 0, 0, nullptr);

  if (m_pImpl->m_hMapping == nullptr || m_pImpl->m_hMapping == INVALID_HANDLE_VALUE)
  {
    errorCode = GetLastError();

    plLog::Error("Could not create memory mapping of file - {}", plArgErrorCode(errorCode));
    Close();
    return PLASMA_FAILURE;
  }

  m_pImpl->m_pMappedFilePtr = MapViewOfFile(m_pImpl->m_hMapping, mode == Mode::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);

  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    errorCode = GetLastError();

    plLog::Error("Could not create memory mapping view of file - {}", plArgErrorCode(errorCode));
    Close();
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plMemoryMappedFile::OpenShared(plStringView sSharedName, plUInt64 uiSize, Mode mode)
{
  PLASMA_ASSERT_DEV(mode != Mode::None, "Invalid mode to open the memory mapped file");
  PLASMA_ASSERT_DEV(uiSize > 0, "plMemoryMappedFile::OpenShared() needs a valid file size to map");

  PLASMA_LOG_BLOCK("MemoryMapFile", sSharedName);

  Close();

  m_pImpl->m_Mode = mode;

  DWORD errorCode = 0;
  DWORD sizeHigh = static_cast<DWORD>((uiSize >> 32) & 0xFFFFFFFFu);
  DWORD sizeLow = static_cast<DWORD>(uiSize & 0xFFFFFFFFu);

  m_pImpl->m_hMapping = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, m_pImpl->m_Mode == Mode::ReadOnly ? PAGE_READONLY : PAGE_READWRITE, sizeHigh,
    sizeLow, plStringWChar(sSharedName).GetData());

  if (m_pImpl->m_hMapping == nullptr || m_pImpl->m_hMapping == INVALID_HANDLE_VALUE)
  {
    errorCode = GetLastError();

    plLog::Error("Could not create memory mapping of file - {}", plArgErrorCode(errorCode));
    Close();
    return PLASMA_FAILURE;
  }

  m_pImpl->m_pMappedFilePtr = MapViewOfFile(m_pImpl->m_hMapping, mode == Mode::ReadOnly ? FILE_MAP_READ : FILE_MAP_WRITE, 0, 0, 0);

  if (m_pImpl->m_pMappedFilePtr == nullptr)
  {
    errorCode = GetLastError();

    plLog::Error("Could not create memory mapping view of file - {}", plArgErrorCode(errorCode));
    Close();
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

void plMemoryMappedFile::Close()
{
  m_pImpl = PLASMA_DEFAULT_NEW(plMemoryMappedFileImpl);
}

plMemoryMappedFile::Mode plMemoryMappedFile::GetMode() const
{
  return m_pImpl->m_Mode;
}

const void* plMemoryMappedFile::GetReadPointer(plUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/) const
{
  PLASMA_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadOnly, "File must be opened with read access before accessing it for reading.");
  PLASMA_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return plMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(uiOffset));
  }
  else
  {
    return plMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(m_pImpl->m_uiFileSize - uiOffset));
  }
}

void* plMemoryMappedFile::GetWritePointer(plUInt64 uiOffset /*= 0*/, OffsetBase base /*= OffsetBase::Start*/)
{
  PLASMA_ASSERT_DEBUG(m_pImpl->m_Mode >= Mode::ReadWrite, "File must be opened with read/write access before accessing it for writing.");
  PLASMA_ASSERT_DEBUG(uiOffset <= m_pImpl->m_uiFileSize, "Read offset must be smaller than mapped file size");

  if (base == OffsetBase::Start)
  {
    return plMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(uiOffset));
  }
  else
  {
    return plMemoryUtils::AddByteOffset(m_pImpl->m_pMappedFilePtr, static_cast<std::ptrdiff_t>(m_pImpl->m_uiFileSize - uiOffset));
  }
}

plUInt64 plMemoryMappedFile::GetFileSize() const
{
  return m_pImpl->m_uiFileSize;
}
