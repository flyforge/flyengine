#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>

plString64 plOSFile::s_sApplicationPath;
plString64 plOSFile::s_sUserDataPath;
plString64 plOSFile::s_sTempDataPath;
plString64 plOSFile::s_sUserDocumentsPath;
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

  PL_ASSERT_DEV(openMode >= plFileOpenMode::Read && openMode <= plFileOpenMode::Append, "Invalid Mode");
  PL_ASSERT_DEV(!IsOpen(), "The file has already been opened.");

  const plTime t0 = plTime::Now();

  m_sFileName = sFile;
  m_sFileName.MakeCleanPath();
  m_sFileName.MakePathSeparatorsNative();

  plResult Res = PL_FAILURE;

  if (!m_sFileName.IsAbsolutePath())
    goto done;

  {
    plStringBuilder sFolder = m_sFileName.GetFileDirectory();

    if (openMode == plFileOpenMode::Write || openMode == plFileOpenMode::Append)
    {
      PL_SUCCEED_OR_RETURN(CreateDirectoryStructure(sFolder.GetData()));
    }
  }

  if (InternalOpen(m_sFileName.GetData(), openMode, fileShareMode) == PL_SUCCESS)
  {
    m_FileMode = openMode;
    Res = PL_SUCCESS;
    goto done;
  }

  m_sFileName.Clear();
  m_FileMode = plFileOpenMode::None;
  goto done;

done:
  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PL_SUCCESS;
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
  PL_ASSERT_DEV((m_FileMode == plFileOpenMode::Write) || (m_FileMode == plFileOpenMode::Append), "The file is not opened for writing.");
  PL_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

  const plTime t0 = plTime::Now();

  const plResult Res = InternalWrite(pBuffer, uiBytes);

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PL_SUCCESS;
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
  PL_ASSERT_DEV(m_FileMode == plFileOpenMode::Read, "The file is not opened for reading.");
  PL_ASSERT_DEV(pBuffer != nullptr, "pBuffer must not be nullptr.");

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
  PL_ASSERT_DEV(m_FileMode == plFileOpenMode::Read, "The file is not opened for reading.");

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
  PL_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");

  return InternalGetFilePosition();
}

void plOSFile::SetFilePosition(plInt64 iDistance, plFileSeekMode::Enum pos) const
{
  PL_ASSERT_DEV(IsOpen(), "The file must be open to tell the file pointer position.");
  PL_ASSERT_DEV(m_FileMode != plFileOpenMode::Append, "SetFilePosition is not possible on files that were opened for appending.");

  return InternalSetFilePosition(iDistance, pos);
}

plUInt64 plOSFile::GetFileSize() const
{
  PL_ASSERT_DEV(IsOpen(), "The file must be open to tell the file size.");

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

  PL_ASSERT_DEV(s.IsAbsolutePath(), "Path must be absolute");

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

void plOSFile::FindFreeFilename(plStringBuilder& inout_sPath, plStringView sSuffix /*= {}*/)
{
  PL_ASSERT_DEV(!inout_sPath.IsEmpty() && inout_sPath.IsAbsolutePath(), "Invalid input path.");

  if (!plOSFile::ExistsFile(inout_sPath))
    return;

  const plString orgName = inout_sPath.GetFileName();

  plStringBuilder newName;

  for (plUInt32 i = 1; i < 100000; ++i)
  {
    newName.SetFormat("{}{}{}", orgName, sSuffix, i);

    inout_sPath.ChangeFileName(newName);
    if (!plOSFile::ExistsFile(inout_sPath))
      return;
  }

  PL_REPORT_FAILURE("Something went wrong.");
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
  e.m_bSuccess = Res == PL_SUCCESS;
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

  PL_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  plStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  plResult Res = PL_SUCCESS;

  while (it.IsValid())
  {
    while ((it.GetCharacter() != '\0') && (!plPathUtils::IsPathSeparator(it.GetCharacter())))
    {
      sCurPath.Append(it.GetCharacter());
      ++it;
    }

    sCurPath.Append(it.GetCharacter());
    ++it;

    if (InternalCreateDirectory(sCurPath.GetData()) == PL_FAILURE)
    {
      Res = PL_FAILURE;
      break;
    }
  }

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PL_SUCCESS;
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

  plResult Res = PL_FAILURE;

  if (SrcFile.Open(sSource, plFileOpenMode::Read) == PL_FAILURE)
    goto done;

  DstFile.m_bRetryOnSharingViolation = false;
  if (DstFile.Open(sDestination, plFileOpenMode::Write) == PL_FAILURE)
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

      if (DstFile.Write(&TempBuffer[0], uiRead) == PL_FAILURE)
        goto done;
    }
  }

  Res = PL_SUCCESS;

done:

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PL_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sSource;
  e.m_sFile2 = sDestination;
  e.m_EventType = EventType::FileCopy;

  s_FileEvents.Broadcast(e);

  return Res;
}

#if PL_ENABLED(PL_SUPPORTS_FILE_STATS)

plResult plOSFile::GetFileStats(plStringView sFileOrFolder, plFileStats& out_stats)
{
  const plTime t0 = plTime::Now();

  plStringBuilder s = sFileOrFolder;
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  PL_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  const plResult Res = InternalGetFileStats(s.GetData(), out_stats);

  const plTime t1 = plTime::Now();
  const plTime tdiff = t1 - t0;

  EventData e;
  e.m_bSuccess = Res == PL_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFileOrFolder;
  e.m_EventType = EventType::FileStat;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  if PL_ENABLED(PL_SUPPORTS_CASE_INSENSITIVE_PATHS) && PL_ENABLED(PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS)
plResult plOSFile::GetFileCasing(plStringView sFileOrFolder, plStringBuilder& out_sCorrectSpelling)
{
  /// \todo We should implement this also on plFileSystem, to be able to support stats through virtual filesystems

  const plTime t0 = plTime::Now();

  plStringBuilder s(sFileOrFolder);
  s.MakeCleanPath();
  s.MakePathSeparatorsNative();

  PL_ASSERT_DEV(s.IsAbsolutePath(), "The path '{0}' is not absolute.", s);

  plStringBuilder sCurPath;

  auto it = s.GetIteratorFront();

  out_sCorrectSpelling.Clear();

  plResult Res = PL_SUCCESS;

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
      if (GetFileStats(sCurPath.GetData(), stats) == PL_FAILURE)
      {
        Res = PL_FAILURE;
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
  e.m_bSuccess = Res == PL_SUCCESS;
  e.m_Duration = tdiff;
  e.m_iFileID = s_iFileCounter.Increment();
  e.m_sFile = sFileOrFolder;
  e.m_EventType = EventType::FileCasing;

  s_FileEvents.Broadcast(e);

  return Res;
}

#  endif // PL_SUPPORTS_CASE_INSENSITIVE_PATHS && PL_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // PL_SUPPORTS_FILE_STATS

#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS) && PL_ENABLED(PL_SUPPORTS_FILE_STATS)

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
      return PL_FAILURE; // unexpected to ever fail, but don't want to assert on it

    dstPath = sDestinationFolder;
    dstPath.AppendPath(relPath);

    if (item.m_bIsDirectory)
    {
      if (plOSFile::CreateDirectoryStructure(dstPath).Failed())
        return PL_FAILURE;
    }
    else
    {
      if (plOSFile::CopyFile(srcPath, dstPath).Failed())
        return PL_FAILURE;

      if (out_pFilesCopied)
      {
        out_pFilesCopied->PushBack(dstPath);
      }
    }

    // TODO: make sure to remove read-only flags of copied files ?
  }

  return PL_SUCCESS;
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
      return PL_FAILURE;
  }

  for (plUInt32 i = items.GetCount(); i > 0; --i)
  {
    const auto& item = items[i - 1];

    if (!item.m_bIsDirectory)
      continue;

    fullPath = item.m_sParentPath;
    fullPath.AppendPath(item.m_sName);

    if (plOSFile::InternalDeleteDirectory(fullPath).Failed())
      return PL_FAILURE;
  }

  if (plOSFile::InternalDeleteDirectory(sFolder).Failed())
    return PL_FAILURE;

  return PL_SUCCESS;
}

#endif // PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS) && PL_ENABLED(PL_SUPPORTS_FILE_STATS)

#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)

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
  PL_ASSERT_DEBUG(m_Flags.IsSet(plFileSystemIteratorFlags::Recursive), "SkipFolder has no meaning when the iterator is not set to be recursive.");
  PL_ASSERT_DEBUG(m_CurFile.m_bIsDirectory, "SkipFolder can only be called when the current object is a folder.");

  m_Flags.Remove(plFileSystemIteratorFlags::Recursive);

  Next();

  m_Flags.Add(plFileSystemIteratorFlags::Recursive);
}

#endif


