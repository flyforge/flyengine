#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>

plString64 plOSFile::s_sApplicationPath;
plString64 plOSFile::s_sUserDataPath;
plString64 plOSFile::s_sTempDataPath;
plAtomicInteger32 plOSFile::s_iFileCounter;
plOSFile::Event plOSFile::s_FileEvents;

plFileStats::plFileStats() = default;
plFileStats::~plFileStats() = default;

void plFileStats::GetFullPath(plStringBuilder& ref_sPath) const
{
  ref_sPath.Set(m_sParentPath, "/", m_sName);
  ref_sPath.MakeCleanPath();
}

plOSFile::plOSFile()
{
  m_FileMode = plFileOpenMode::None;
  m_iFileID = s_iFileCounter.Increment();
}

plOSFile::~plOSFile()
{
  Close();
}

plResult plOSFile::Open(plStringView sFile, plFileOpenMode::Enum openMode, plFileShareMode::Enum fileShareMode)
{
  m_iFileID = s_iFileCounter.Increment();

  PLASMA_ASSERT_DEV(openMode >= plFileOpenMode::Read && openMode <= plFileOpenMode::Append, "Invalid Mode");
  PLASMA_ASSERT_DEV(!IsOpen(), "The file has already been opened.");

  const plTime t0 = plTime::Now();

  m_sFileName = sFile;
  m_sFileName.MakeCleanPath();
  m_sFileName.MakePathSeparatorsNative();

  plResult Res = PLASMA_FAILURE;

  if (!m_sFileName.IsAbsolutePath())
    goto done;

  {
    plStringBuilder sFolder = m_sFileName.GetFileDirectory();

    if (openMode == plFileOpenMode::Write || openMode == plFileOpenMode::Append)
    {
      PLASMA_SUCCEED_OR_RETURN(CreateDirectoryStructure(sFolder.GetData()));
    }
  }

  if (InternalOpen(m_sFileName.GetData(), openMode, fileShareMode) == PLASMA_SUCCESS)
  {
    m_FileMode = openMode;
    Res = PLASMA_SUCCESS;
    goto done;
  }

  m_sFileName.Clear();
  m_FileMode = plFileOpenMode::None;
  goto done;

done:
  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PLASMA_SUCCESS;
  e.m_Duration = tdiff;
  e.m_FileMode = openMode;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileOpen;

  s_FileEvents.Broadcast(e);

  return Res;
}

bool plOSFile::IsOpen() const
{
  return m_FileMode != plFileOpenMode::None;
}

void plOSFile::Close()
{
  if (!IsOpen())
    return;

  const plTime t0 = plTime::Now();

  InternalClose();

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = true;
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileClose;

  s_FileEvents.Broadcast(e);

  m_sFileName.Clear();
  m_FileMode = plFileOpenMode::None;
}

plResult plOSFile::Write(const void* pBuffer, plUInt64 uiBytes)
{
  PLASMA_ASSERT_DEV((m_FileMode == plFileOpenMode::Write) || (m_FileMode == plFileOpenMode::Append), "The file is not opened for writing.");
  PLASMA_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

  const plTime t0 = plTime::Now();

  const plResult Res = InternalWrite(pBuffer, uiBytes);

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PLASMA_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileWrite;
  e.m_uiBytesAccessed = uiBytes;

  s_FileEvents.Broadcast(e);

  return Res;
}

plUInt64 plOSFile::Read(void* pBuffer, plUInt64 uiBytes)
{
  PLASMA_ASSERT_DEV(m_FileMode == plFileOpenMode::Read, "The file is not opened for reading.");
  PLASMA_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

  const plTime t0 = plTime::Now();

  const plUInt64 Res = InternalRead(pBuffer, uiBytes);

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = (Res == uiBytes);
  e.m_Duration = tdiff;
  e.m_iFileID = m_iFileID;
  e.m_sFile = m_sFileName;
  e.m_EventType = EventType::FileRead;
  e.m_uiBytesAccessed = Res;

  s_FileEvents.Broadcast(e);

  return Res;
}

plUInt64 plOSFile::ReadAll(plDynamicArray<plUInt8>& out_fileContent)
{
  PLASMA_ASSERT_DEV(m_FileMode == plFileOpenMode::Read, "The file is not opened for reading.");

  out_fileContent.Clear();
  out_fileContent.SetCountUninitialized((plUInt32)GetFileSize());

  if (!out_fileContent.IsEmpty())
  {
    Read(out_fileContent.GetData(), out_fileContent.GetCount());
  }

  return out_fileContent.GetCount();
}

plUInt64 plOSFile::GetFilePosition() const
{
  PLASMA_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void plOSFile::SetFilePosition(plInt64 iDistance, plFileSeekMode::Enum pos) const
{
  PLASMA_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");
  PLASMA_ASSERT_DEV(m_FileMode != plFileOpenMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, pos);
}

plUInt64 plOSFile::GetFileSize() const
{
  PLASMA_ASSERT_DEV(IsOpen(), "The file must be open to tell the file size.");

  const plInt64 iCurPos = static_cast<plInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(0, plFileSeekMode::FromEnd);

  const plUInt64 uiCurSize = static_cast<plInt64>(GetFilePosition());

  // to circumvent the 'append does not support SetFilePosition' assert, we use the internal function directly
  InternalSetFilePosition(iCurPos, plFileSeekMode::FromStart);

  return uiCurSize;
}

const plString plOSFile::MakePathAbsoluteWithCWD(plStringView sPath)
{
  plStringBuilder tmp = sPath;
  tmp.MakeCleanPath();

  if (tmp.IsRelativePath())
  {
    tmp.PrependFormat("{}/", GetCurrentWorkingDirectory());
    tmp.MakeCleanPath();
  }

  return tmp;
}

bool plOSFile::ExistsFile(plStringView sFile)
{
  const plTime t0 = plTime::Now();

  plStringBuilder s(sFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const bool bRes = InternalExistsFile(s);

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;


  EventData e;
  e.m_bSuccess = bRes;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = s;
  e.m_EventType = EventType::FileExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

bool plOSFile::ExistsDirectory(plStringView sDirectory)
{
  const plTime t0 = plTime::Now();

  plStringBuilder s(sDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  PLASMA_ASSERT_DEV(s.IsAbsolutePath(), "Path must be absolute");

  const bool bRes = InternalExistsDirectory(s);

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;


  EventData e;
  e.m_bSuccess = bRes;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = s;
  e.m_EventType = EventType::DirectoryExists;

  s_FileEvents.Broadcast(e);

  return bRes;
}

plResult plOSFile::DeleteFile(plStringView sFile)
{
  const plTime t0 = plTime::Now();

  plStringBuilder s(sFile);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  const plResult Res = InternalDeleteFile(s.GetData());

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PLASMA_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFile;
  e.m_EventType = EventType::FileDelete;

  s_FileEvents.Broadcast(e);

  return Res;
}

plResult plOSFile::CreateDirectoryStructure(plStringView sDirectory)
{
  const plTime t0 = plTime::Now();

  plStringBuilder s(sDirectory);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  PLASMA_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  plStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  plResult Res = PLASMA_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!plPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    sCurPath.Append(it.GetCharacter());
    ++it;

    if (InternalCreateDirectory(sCurPath.GetData()) == PLASMA_FAILURE)
    {
      Res = PLASMA_FAILURE;
      break;
    }
  }

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PLASMA_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sDirectory;
  e.m_EventType = EventType::MakeDir;

  s_FileEvents.Broadcast(e);

  return Res;
}

plResult plOSFile::MoveFileOrDirectory(plStringView sDirectoryFrom, plStringView sDirectoryTo)
{
  plStringBuilder sFrom(sDirectoryFrom);
  sFrom.MakeCleanPath();
  sFrom.MakePathSeparatorsNative();

  plStringBuilder sTo(sDirectoryTo);
  sTo.MakeCleanPath();
  sTo.MakePathSeparatorsNative();

  return InternalMoveFileOrDirectory(sFrom, sTo);
}

plResult plOSFile::CopyFile(plStringView sSource, plStringView sDestination)
{
  const plTime t0 = plTime::Now();

  plOSFile SrcFile, DstFile;

  plResult Res = PLASMA_FAILURE;

  if (SrcFile.Open(sSource, plFileOpenMode::Read) == PLASMA_FAILURE)
    goto done;

  DstFile.m_bRetryOnSharingViolation = false;
  if (DstFile.Open(sDestination, plFileOpenMode::Write) == PLASMA_FAILURE)
    goto done;

  {
    const plUInt32 uiTempSize = 1024 * 1024 * 8; // 8 MB

    // can't allocate that much data on the stack
    plDynamicArray<plUInt8> TempBuffer;
    TempBuffer.SetCountUninitialized(uiTempSize);

    while (true)
    {
      const plUInt64 uiRead = SrcFile.Read(&TempBuffer[0], uiTempSize);

      if (uiRead == 0)
        break;

      if (DstFile.Write(&TempBuffer[0], uiRead) == PLASMA_FAILURE)
        goto done;
    }
  }

  Res = PLASMA_SUCCESS;

done:

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PLASMA_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sSource;
  e.m_sFile2 = sDestination;
  e.m_EventType = EventType::FileCopy;

  s_FileEvents.Broadcast(e);

  return Res;
}

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)

plResult plOSFile::GetFileStats(plStringView sFileOrFolder, plFileStats& out_stats)
{
  const plTime t0 = plTime::Now();

  plStringBuilder s = sFileOrFolder;
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  PLASMA_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  const plResult Res = InternalGetFileStats(s.GetData(), out_stats);

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PLASMA_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFileOrFolder;
  e.m_EventType = EventType::FileStat;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  if PLASMA_ENABLED(PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS) && PLASMA_ENABLED(PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
plResult plOSFile::GetFileCasing(plStringView sFileOrFolder, plStringBuilder& out_sCorrectSpelling)
{
  /// \todo We should implement this also on plFileSystem, to be able to support stats through virtual filesystems

  const plTime t0 = plTime::Now();

  plStringBuilder s(sFileOrFolder);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  PLASMA_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  plStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  out_sCorrectSpelling.Clear();

  plResult Res = PLASMA_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!plPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    if (!sCurPath.IsEmpty())
    {
      plFileStats stats;
      if (GetFileStats(sCurPath.GetData(), stats) == PLASMA_FAILURE)
      {
        Res = PLASMA_FAILURE;
        break;
      }

      out_sCorrectSpelling.AppendPath(stats.m_sName);
    }
    sCurPath.Append(it.GetCharacter());
    ++it;
  }

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PLASMA_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFileOrFolder;
  e.m_EventType = EventType::FileCasing;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  endif // PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS && PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // PLASMA_SUPPORTS_FILE_STATS

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS) && PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)

void plOSFile::GatherAllItemsInFolder(plDynamicArray<plFileStats>& out_itemList, plStringView sFolder, plBitflags<plFileSystemIteratorFlags> flags /*= plFileSystemIteratorFlags::All*/)
{
  out_itemList.Clear();

  plFileSystemIterator iterator;
  iterator.StartSearch(sFolder, flags);

  if (!iterator.IsValid())
    return;

  out_itemList.Reserve(128);

  while (iterator.IsValid())
  {
    out_itemList.PushBack(iterator.GetStats());

    iterator.Next();
  }
}

plResult plOSFile::CopyFolder(plStringView sSourceFolder, plStringView sDestinationFolder, plDynamicArray<plString>* out_pFilesCopied /*= nullptr*/)
{
  plDynamicArray<plFileStats> items;
  GatherAllItemsInFolder(items, sSourceFolder);

  plStringBuilder srcPath;
  plStringBuilder dstPath;
  plStringBuilder relPath;

  for (const auto& item : items)
  {
    srcPath = item.m_sParentPath;
    srcPath.AppendPath(item.m_sName);

    relPath = srcPath;

    if (relPath.MakeRelativeTo(sSourceFolder).Failed())
      return PLASMA_FAILURE; // unexpected to ever fail, but don't want to assert on it

    dstPath = sDestinationFolder;
    dstPath.AppendPath(relPath);

    if (item.m_bIsDirectory)
    {
      if (plOSFile::CreateDirectoryStructure(dstPath).Failed())
        return PLASMA_FAILURE;
    }
    else
    {
      if (plOSFile::CopyFile(srcPath, dstPath).Failed())
        return PLASMA_FAILURE;

      if (out_pFilesCopied)
      {
        out_pFilesCopied->PushBack(dstPath);
      }
    }

    // TODO: make sure to remove read-only flags of copied files ?
  }

  return PLASMA_SUCCESS;
}

plResult plOSFile::DeleteFolder(plStringView sFolder)
{
  plDynamicArray<plFileStats> items;
  GatherAllItemsInFolder(items, sFolder);

  plStringBuilder fullPath;

  for (const auto& item : items)
  {
    if (item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (plOSFile::DeleteFile(fullPath).Failed())
      return PLASMA_FAILURE;
  }

  for (plUInt32 i = items.GetCount(); i > 0; --i)
  {
    const auto& item = items[i - 1];

    if (!item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (plOSFile::InternalDeleteDirectory(fullPath).Failed())
      return PLASMA_FAILURE;
  }

  if (plOSFile::InternalDeleteDirectory(sFolder).Failed())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

#endif // PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS) && PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)

void plFileSystemIterator::StartMultiFolderSearch(plArrayPtr<plString> startFolders, plStringView sSearchTerm, plBitflags<plFileSystemIteratorFlags> flags /*= plFileSystemIteratorFlags::Default*/)
{
  if (startFolders.IsEmpty())
    return;

  m_sMultiSearchTerm = sSearchTerm;
  m_Flags = flags;
  m_uiCurrentStartFolder = 0;
  m_StartFolders = startFolders;

  plStringBuilder search = startFolders[m_uiCurrentStartFolder];
  search.AppendPath(sSearchTerm);

  StartSearch(search, m_Flags);

  if (!IsValid())
  {
    Next();
  }
}

void plFileSystemIterator::Next()
{
  while (true)
  {
    const plInt32 res = InternalNext();

    if (res == 1) // success
    {
      return;
    }
    else if (res == 0) // failure
    {
      ++m_uiCurrentStartFolder;

      if (m_uiCurrentStartFolder < m_StartFolders.GetCount())
      {
        plStringBuilder search = m_StartFolders[m_uiCurrentStartFolder];
        search.AppendPath(m_sMultiSearchTerm);

        StartSearch(search, m_Flags);
      }
      else
      {
        return;
      }

      if (IsValid())
      {
        return;
      }
    }
    else
    {
      // call InternalNext() again
    }
  }
}

void plFileSystemIterator::SkipFolder()
{
  PLASMA_ASSERT_DEBUG(m_Flags.IsSet(plFileSystemIteratorFlags::Recursive), "SkipFolder has no meaning when the iterator is not set to be recursive.");
  PLASMA_ASSERT_DEBUG(m_CurFile.m_bIsDirectory, "SkipFolder can only be called when the current object is a folder.");

  m_Flags.Remove(plFileSystemIteratorFlags::Recursive);

  Next();

  m_Flags.Add(plFileSystemIteratorFlags::Recursive);
}

#endif


#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/IO/Implementation/Win/OSFile_win.h>

// For UWP we're currently using a mix of WinRT functions and posix.
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#    include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#  endif
#elif PLASMA_ENABLED(PLASMA_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/OSFile_posix.h>
#else
#  error "Unknown Platform."
#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OSFile);
