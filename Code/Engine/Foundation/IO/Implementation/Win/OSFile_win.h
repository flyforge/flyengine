#pragma once

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/Implementation/Win/DosDevicePath_win.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Threading/ThreadUtils.h>

// Defined in Timestamp_win.h
plInt64 FileTimeToEpoch(FILETIME fileTime);

static plUInt64 HighLowToUInt64(plUInt32 uiHigh32, plUInt32 uiLow32)
{
  plUInt64 uiHigh64 = uiHigh32;
  plUInt64 uiLow64 = uiLow32;

  return (uiHigh64 << 32) | uiLow64;
}

#if PLASMA_DISABLED(PLASMA_USE_POSIX_FILE_API)

#  include <Shlobj.h>

plResult plOSFile::InternalOpen(plStringView sFile, plFileOpenMode::Enum OpenMode, plFileShareMode::Enum FileShareMode)
{
  const plTime sleepTime = plTime::Milliseconds(20);
  plInt32 iRetries = 20;

  if (FileShareMode == plFileShareMode::Default)
  {
    // when 'default' share mode is requested, use 'share reads' when opening a file for reading
    // and use 'exclusive' when opening a file for writing

    if (OpenMode == plFileOpenMode::Read)
    {
      FileShareMode = plFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = plFileShareMode::Exclusive;
    }
  }

  DWORD dwSharedMode = 0; // exclusive access
  if (FileShareMode == plFileShareMode::SharedReads)
  {
    dwSharedMode = FILE_SHARE_READ;
  }

  while (iRetries > 0)
  {
    SetLastError(ERROR_SUCCESS);
    DWORD error = 0;

    switch (OpenMode)
    {
      case plFileOpenMode::Read:
        m_FileData.m_pFileHandle = CreateFileW(plDosDevicePath(sFile), GENERIC_READ, dwSharedMode, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case plFileOpenMode::Write:
        m_FileData.m_pFileHandle = CreateFileW(plDosDevicePath(sFile), GENERIC_WRITE, dwSharedMode, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        break;

      case plFileOpenMode::Append:
        m_FileData.m_pFileHandle = CreateFileW(plDosDevicePath(sFile), FILE_APPEND_DATA, dwSharedMode, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

        // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
        if ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE))
          InternalSetFilePosition(0, plFileSeekMode::FromEnd);

        break;

        PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED
    }

    const plResult res = ((m_FileData.m_pFileHandle != nullptr) && (m_FileData.m_pFileHandle != INVALID_HANDLE_VALUE)) ? PLASMA_SUCCESS : PLASMA_FAILURE;

    if (res.Failed())
    {
      if (plOSFile::ExistsDirectory(sFile))
      {
        // trying to 'open' a directory fails with little useful error codes such as 'access denied'
        return PLASMA_FAILURE;
      }

      error = GetLastError();

      // file does not exist
      if (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)
        return res;
      // badly formed path, happens when two absolute paths are concatenated
      if (error == ERROR_INVALID_NAME)
        return res;

      if (error == ERROR_SHARING_VIOLATION
          // these two situations happen when the plInspector is connected
          // for some reason, the networking blocks file reading (when run on the same machine)
          // retrying fixes the problem, but can introduce very long stalls
          || error == WSAEWOULDBLOCK || error == ERROR_SUCCESS)
      {
        if (m_bRetryOnSharingViolation)
        {
          --iRetries;
          plThreadUtils::Sleep(sleepTime);
          continue; // try again
        }
        else
        {
          return res;
        }
      }

      // anything else, print an error (for now)
      plLog::Error("CreateFile failed with error {0}", plArgErrorCode(error));
    }

    return res;
  }

  return PLASMA_FAILURE;
}

void plOSFile::InternalClose()
{
  CloseHandle(m_FileData.m_pFileHandle);
  m_FileData.m_pFileHandle = INVALID_HANDLE_VALUE;
}

plResult plOSFile::InternalWrite(const void* pBuffer, plUInt64 uiBytes)
{
  const plUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBatchBytes))
      return PLASMA_FAILURE;

    uiBytes -= uiBatchBytes;
    pBuffer = plMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const plUInt32 uiBytes32 = static_cast<plUInt32>(uiBytes);

    DWORD uiBytesWritten = 0;
    if ((!WriteFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesWritten, nullptr)) || (uiBytesWritten != uiBytes32))
      return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plUInt64 plOSFile::InternalRead(void* pBuffer, plUInt64 uiBytes)
{
  plUInt64 uiBytesRead = 0;

  const plUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBatchBytes, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;

    if (uiBytesReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = plMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const plUInt32 uiBytes32 = static_cast<plUInt32>(uiBytes);

    DWORD uiBytesReadThisTime = 0;
    if (!ReadFile(m_FileData.m_pFileHandle, pBuffer, uiBytes32, &uiBytesReadThisTime, nullptr))
      return uiBytesRead + uiBytesReadThisTime;

    uiBytesRead += uiBytesReadThisTime;
  }

  return uiBytesRead;
}

plUInt64 plOSFile::InternalGetFilePosition() const
{
  long int uiHigh32 = 0;
  plUInt32 uiLow32 = SetFilePointer(m_FileData.m_pFileHandle, 0, &uiHigh32, FILE_CURRENT);

  return HighLowToUInt64(uiHigh32, uiLow32);
}

void plOSFile::InternalSetFilePosition(plInt64 iDistance, plFileSeekMode::Enum Pos) const
{
  LARGE_INTEGER pos;
  LARGE_INTEGER newpos;
  pos.QuadPart = static_cast<LONGLONG>(iDistance);

  switch (Pos)
  {
    case plFileSeekMode::FromStart:
      PLASMA_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_BEGIN), "Seek Failed.");
      break;
    case plFileSeekMode::FromEnd:
      PLASMA_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_END), "Seek Failed.");
      break;
    case plFileSeekMode::FromCurrent:
      PLASMA_VERIFY(SetFilePointerEx(m_FileData.m_pFileHandle, pos, &newpos, FILE_CURRENT), "Seek Failed.");
      break;
  }
}

bool plOSFile::InternalExistsFile(plStringView sFile)
{
  const DWORD dwAttrib = GetFileAttributesW(plDosDevicePath(sFile).GetData());

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0));
}

bool plOSFile::InternalExistsDirectory(plStringView sDirectory)
{
  const DWORD dwAttrib = GetFileAttributesW(plDosDevicePath(sDirectory));

  return ((dwAttrib != INVALID_FILE_ATTRIBUTES) && ((dwAttrib & FILE_ATTRIBUTE_DIRECTORY) != 0));
}

plResult plOSFile::InternalDeleteFile(plStringView sFile)
{
  if (DeleteFileW(plDosDevicePath(sFile)) == FALSE)
  {
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
      return PLASMA_SUCCESS;

    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plOSFile::InternalDeleteDirectory(plStringView sDirectory)
{
  if (RemoveDirectoryW(plDosDevicePath(sDirectory)) == FALSE)
  {
    if (GetLastError() == ERROR_FILE_NOT_FOUND)
      return PLASMA_SUCCESS;

    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plOSFile::InternalCreateDirectory(plStringView sDirectory)
{
  // handle drive letters as always successful
  if (plStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 3) // 'C:\'
    return PLASMA_SUCCESS;

  if (CreateDirectoryW(plDosDevicePath(sDirectory), nullptr) == FALSE)
  {
    const DWORD uiError = GetLastError();
    if (uiError == ERROR_ALREADY_EXISTS)
      return PLASMA_SUCCESS;

    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

plResult plOSFile::InternalMoveFileOrDirectory(plStringView sDirectoryFrom, plStringView sDirectoryTo)
{
  if (MoveFileW(plDosDevicePath(sDirectoryFrom), plDosDevicePath(sDirectoryTo)) == 0)
  {
    return PLASMA_FAILURE;
  }
  return PLASMA_SUCCESS;
}

#endif // not PLASMA_USE_POSIX_FILE_API

plResult plOSFile::InternalGetFileStats(plStringView sFileOrFolder, plFileStats& out_Stats)
{
  plStringBuilder s = sFileOrFolder;

  // FindFirstFile does not like paths that end with a separator, so remove them all
  s.Trim(nullptr, "/\\");

  // handle the case that this query is done on the 'device part' of a path
  if (s.GetCharacterCount() <= 2) // 'C:', 'D:', 'E' etc.
  {
    s.ToUpper();

    out_Stats.m_uiFileSize = 0;
    out_Stats.m_bIsDirectory = true;
    out_Stats.m_sParentPath.Clear();
    out_Stats.m_sName = s;
    out_Stats.m_LastModificationTime.Invalidate();
    return PLASMA_SUCCESS;
  }

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(plDosDevicePath(s), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return PLASMA_FAILURE;

  out_Stats.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  out_Stats.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = data.cFileName;
  out_Stats.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), plSIUnitOfTime::Microsecond);

  FindClose(hSearch);
  return PLASMA_SUCCESS;
}

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)

plFileSystemIterator::plFileSystemIterator() = default;

plFileSystemIterator::~plFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    FindClose(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

bool plFileSystemIterator::IsValid() const
{
  return !m_Data.m_Handles.IsEmpty();
}

void plFileSystemIterator::StartSearch(plStringView sSearchStart, plBitflags<plFileSystemIteratorFlags> flags /*= plFileSystemIteratorFlags::All*/)
{
  PLASMA_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  m_sSearchTerm = sSearchStart;

  plStringBuilder sSearch = sSearchStart;
  sSearch.MakeCleanPath();

  // same as just passing in the folder path, so remove this
  if (sSearch.EndsWith("/*"))
    sSearch.Shrink(0, 2);

  // The Windows documentation disallows trailing (back)slashes.
  sSearch.Trim(nullptr, "/");

  // Since the use of wildcard-ed file names will disable recursion, we ensure both are not used simultaneously.
  const bool bHasWildcard = sSearch.FindLastSubString("*") || sSearch.FindLastSubString("?");
  PLASMA_ASSERT_DEV(flags.IsSet(plFileSystemIteratorFlags::Recursive) == false || bHasWildcard == false, "Recursive file iteration does not support wildcards. Either don't use recursion, or filter the filenames manually.");

  m_sCurPath = sSearch.GetFileDirectory();

  PLASMA_ASSERT_DEV(sSearch.IsAbsolutePath(), "The path '{0}' is not absolute.", m_sCurPath);

  m_Flags = flags;

  WIN32_FIND_DATAW data;
  HANDLE hSearch = FindFirstFileW(plDosDevicePath(sSearch), &data);

  if ((hSearch == nullptr) || (hSearch == INVALID_HANDLE_VALUE))
    return;

  m_CurFile.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sParentPath = m_sCurPath;
  m_CurFile.m_sName = data.cFileName;
  m_CurFile.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), plSIUnitOfTime::Microsecond);

  m_Data.m_Handles.PushBack(hSearch);

  if (plOSFile::ExistsDirectory(sSearch))
  {
    // when calling FindFirstFileW with a path to a folder (e.g. "C:/test") it will report "test" as the very first item
    // which is typically NOT what one wants, instead you want items INSIDE that folder to be reported
    // this is especially annoying when 'Recursion' is disabled, as "C:/test" would result in "C:/test" being reported
    // but no items inside it
    // therefore, when the start search points to a directory, we enable recursion for one call to 'Next', thus enter
    // the directory, and then switch it back again; all following calls to 'Next' will then iterate through the sub directory

    const bool bRecursive = m_Flags.IsSet(plFileSystemIteratorFlags::Recursive);
    m_Flags.Add(plFileSystemIteratorFlags::Recursive);

    Next();

    m_Flags.AddOrRemove(plFileSystemIteratorFlags::Recursive, bRecursive);
    return;
  }

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
  {
    Next(); // will search for the next file or folder that is not ".." or "." ; might return false though
    return;
  }

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFolders))
    {
      Next();
      return;
    }
  }
  else
  {
    if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFiles))
    {
      Next();
      return;
    }
  }
}

plInt32 plFileSystemIterator::InternalNext()
{
  constexpr plInt32 ReturnFailure = 0;
  constexpr plInt32 ReturnSuccess = 1;
  constexpr plInt32 ReturnCallInternalNext = 2;

  if (m_Data.m_Handles.IsEmpty())
    return ReturnFailure;

  if (m_Flags.IsSet(plFileSystemIteratorFlags::Recursive) && m_CurFile.m_bIsDirectory && (m_CurFile.m_sName != "..") && (m_CurFile.m_sName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sName);

    plStringBuilder sNewSearch = m_sCurPath;
    sNewSearch.AppendPath("*");

    WIN32_FIND_DATAW data;
    HANDLE hSearch = FindFirstFileW(plDosDevicePath(sNewSearch), &data);

    if ((hSearch != nullptr) && (hSearch != INVALID_HANDLE_VALUE))
    {
      m_CurFile.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
      m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
      m_CurFile.m_sParentPath = m_sCurPath;
      m_CurFile.m_sName = data.cFileName;
      m_CurFile.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), plSIUnitOfTime::Microsecond);

      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
        return ReturnCallInternalNext; // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory)
      {
        if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFolders))
          return ReturnCallInternalNext;
      }
      else
      {
        if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFiles))
          return ReturnCallInternalNext;
      }

      return ReturnSuccess;
    }

    // if the recursion did not work, just iterate in this folder further
  }

  WIN32_FIND_DATAW data;
  if (!FindNextFileW(m_Data.m_Handles.PeekBack(), &data))
  {
    // nothing found in this directory anymore
    FindClose(m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();

    if (m_Data.m_Handles.IsEmpty())
      return ReturnFailure;

    m_sCurPath.PathParentDirectory();
    if (m_sCurPath.EndsWith("/"))
    {
      m_sCurPath.Shrink(0, 1); // Remove trailing /
    }

    return ReturnCallInternalNext;
  }

  m_CurFile.m_uiFileSize = HighLowToUInt64(data.nFileSizeHigh, data.nFileSizeLow);
  m_CurFile.m_bIsDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
  m_CurFile.m_sParentPath = m_sCurPath;
  m_CurFile.m_sName = data.cFileName;
  m_CurFile.m_LastModificationTime.SetInt64(FileTimeToEpoch(data.ftLastWriteTime), plSIUnitOfTime::Microsecond);

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
    return ReturnCallInternalNext;

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFolders))
      return ReturnCallInternalNext;
  }
  else
  {
    if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFiles))
      return ReturnCallInternalNext;
  }

  return ReturnSuccess;
}

#endif

plStringView plOSFile::GetApplicationDirectory()
{
  if (s_sApplicationPath.IsEmpty())
  {
    plUInt32 uiRequiredLength = 512;
    plHybridArray<wchar_t, 1024> tmp;

    while (true)
    {
      tmp.SetCountUninitialized(uiRequiredLength);

      // reset last error code
      SetLastError(ERROR_SUCCESS);

      const plUInt32 uiLength = GetModuleFileNameW(nullptr, tmp.GetData(), tmp.GetCount() - 1);
      const DWORD error = GetLastError();

      if (error == ERROR_SUCCESS)
      {
        tmp[uiLength] = L'\0';
        break;
      }

      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
        uiRequiredLength += 512;
        continue;
      }

      PLASMA_REPORT_FAILURE("GetModuleFileNameW failed: {0}", plArgErrorCode(error));
    }

    s_sApplicationPath = plPathUtils::GetFileDirectory(plStringUtf8(tmp.GetData()));
  }

  return s_sApplicationPath;
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <windows.storage.h>
#endif

plString plOSFile::GetUserDataFolder(plStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
    ComPtr<ABI::Windows::Storage::IApplicationDataStatics> appDataStatics;
    if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &appDataStatics)))
    {
      ComPtr<ABI::Windows::Storage::IApplicationData> applicationData;
      if (SUCCEEDED(appDataStatics->get_Current(&applicationData)))
      {
        ComPtr<ABI::Windows::Storage::IStorageFolder> applicationDataLocal;
        if (SUCCEEDED(applicationData->get_LocalFolder(&applicationDataLocal)))
        {
          ComPtr<ABI::Windows::Storage::IStorageItem> localFolderItem;
          if (SUCCEEDED(applicationDataLocal.As(&localFolderItem)))
          {
            HSTRING path;
            localFolderItem->get_Path(&path);
            s_sUserDataPath = plStringUtf8(path).GetData();
          }
        }
      }
    }
#else
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s_sUserDataPath = plStringWChar(pPath);
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
#endif
  }

  plStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

plString plOSFile::GetTempDataFolder(plStringView sSubFolder /*= nullptr*/)
{
  plStringBuilder s;

  if (s_sTempDataPath.IsEmpty())
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
    ComPtr<ABI::Windows::Storage::IApplicationDataStatics> appDataStatics;
    if (SUCCEEDED(ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &appDataStatics)))
    {
      ComPtr<ABI::Windows::Storage::IApplicationData> applicationData;
      if (SUCCEEDED(appDataStatics->get_Current(&applicationData)))
      {
        ComPtr<ABI::Windows::Storage::IStorageFolder> applicationTempData;
        if (SUCCEEDED(applicationData->get_TemporaryFolder(&applicationTempData)))
        {
          ComPtr<ABI::Windows::Storage::IStorageItem> tempFolderItem;
          if (SUCCEEDED(applicationTempData.As(&tempFolderItem)))
          {
            HSTRING path;
            tempFolderItem->get_Path(&path);
            s_sTempDataPath = plStringUtf8(path).GetData();
          }
        }
      }
    }
#else
    wchar_t* pPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_DEFAULT, nullptr, &pPath)))
    {
      s = plStringWChar(pPath);
      s.AppendPath("Temp");
      s_sTempDataPath = s;
    }

    if (pPath != nullptr)
    {
      CoTaskMemFree(pPath);
    }
#endif
  }

  s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

const plString plOSFile::GetCurrentWorkingDirectory()
{
  const plUInt32 uiRequiredLength = GetCurrentDirectoryW(0, nullptr);

  plHybridArray<wchar_t, 1024> tmp;
  tmp.SetCountUninitialized(uiRequiredLength + 16);

  if (GetCurrentDirectoryW(tmp.GetCount() - 1, tmp.GetData()) == 0)
  {
    PLASMA_REPORT_FAILURE("GetCurrentDirectoryW failed: {}", plArgErrorCode(GetLastError()));
    return plString();
  }

  tmp[uiRequiredLength] = L'\0';

  plStringBuilder clean = plStringUtf8(tmp.GetData()).GetData();
  clean.MakeCleanPath();

  return clean;
}
