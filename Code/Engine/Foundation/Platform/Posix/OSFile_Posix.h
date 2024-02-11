#include <Foundation/FoundationPCH.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <direct.h>
#  define PL_USE_OLD_POSIX_FUNCTIONS PL_ON
#else
#  include <dirent.h>
#  include <fnmatch.h>
#  include <pwd.h>
#  include <sys/file.h>
#  include <sys/types.h>
#  include <unistd.h>
#  define PL_USE_OLD_POSIX_FUNCTIONS PL_OFF
#endif

#if PL_ENABLED(PL_PLATFORM_OSX)
#  include <CoreFoundation/CoreFoundation.h>
#endif

#if PL_ENABLED(PL_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidJni.h>
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android_native_app_glue.h>
#endif

#ifndef PATH_MAX
#  define PATH_MAX 1024
#endif

plResult plOSFile::InternalOpen(plStringView sFile, plFileOpenMode::Enum OpenMode, plFileShareMode::Enum FileShareMode)
{
  plStringBuilder sFileCopy = sFile;
  const char* szFile = sFileCopy;

#if PL_DISABLED(PL_PLATFORM_WINDOWS_UWP) // UWP does not support these functions
  int fd = -1;
  switch (OpenMode)
  {
    // O_CLOEXEC = don't forward to child processes
    case plFileOpenMode::Read:
      fd = open(szFile, O_RDONLY | O_CLOEXEC);
      break;
    case plFileOpenMode::Write:
    case plFileOpenMode::Append:
      fd = open(szFile, O_CREAT | O_WRONLY | O_CLOEXEC, 0644);
      break;
    default:
      break;
  }

  if (FileShareMode == plFileShareMode::Default)
  {
    if (OpenMode == plFileOpenMode::Read)
    {
      FileShareMode = plFileShareMode::SharedReads;
    }
    else
    {
      FileShareMode = plFileShareMode::Exclusive;
    }
  }

  if (fd == -1)
  {
    return PL_FAILURE;
  }

  const int iSharedMode = (FileShareMode == plFileShareMode::Exclusive) ? LOCK_EX : LOCK_SH;
  const plTime sleepTime = plTime::MakeFromMilliseconds(20);
  plInt32 iRetries = m_bRetryOnSharingViolation ? 20 : 1;

  while (flock(fd, iSharedMode | LOCK_NB /* do not block */) != 0)
  {
    int errorCode = errno;
    iRetries--;
    if (iRetries == 0 || errorCode != EWOULDBLOCK)
    {
      // error, could not get a lock
      plLog::Error("Failed to get a {} lock for file {}, error {}", (FileShareMode == plFileShareMode::Exclusive) ? "Exculsive" : "Shared", szFile, errno);
      close(fd);
      return PL_FAILURE;
    }
    plThreadUtils::Sleep(sleepTime);
  }

  switch (OpenMode)
  {
    case plFileOpenMode::Read:
      m_FileData.m_pFileHandle = fdopen(fd, "rb");
      break;
    case plFileOpenMode::Write:
      if (ftruncate(fd, 0) < 0)
      {
        close(fd);
        return PL_FAILURE;
      }
      m_FileData.m_pFileHandle = fdopen(fd, "wb");
      break;
    case plFileOpenMode::Append:
      m_FileData.m_pFileHandle = fdopen(fd, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, plFileSeekMode::FromEnd);

      break;
    default:
      break;
  }

  if (m_FileData.m_pFileHandle == nullptr)
  {
    close(fd);
  }

#else

  switch (OpenMode)
  {
    case plFileOpenMode::Read:
      m_FileData.m_pFileHandle = fopen(szFile, "rb");
      break;
    case plFileOpenMode::Write:
      m_FileData.m_pFileHandle = fopen(szFile, "wb");
      break;
    case plFileOpenMode::Append:
      m_FileData.m_pFileHandle = fopen(szFile, "ab");

      // in append mode we need to set the file pointer to the end explicitly, otherwise GetFilePosition might return 0 the first time
      if (m_FileData.m_pFileHandle != nullptr)
        InternalSetFilePosition(0, plFileSeekMode::FromEnd);

      break;
    default:
      break;
  }
#endif

  if (m_FileData.m_pFileHandle == nullptr)
  {
    return PL_FAILURE;
  }

  // lock will be released automatically when the file is closed
  return PL_SUCCESS;
}

void plOSFile::InternalClose()
{
  fclose(m_FileData.m_pFileHandle);
}

plResult plOSFile::InternalWrite(const void* pBuffer, plUInt64 uiBytes)
{
  const plUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    if (fwrite(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle) != uiBatchBytes)
    {
      plLog::Error("fwrite 1GB failed for '{}'", m_sFileName);
      return PL_FAILURE;
    }

    uiBytes -= uiBatchBytes;
    pBuffer = plMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const plUInt32 uiBytes32 = static_cast<plUInt32>(uiBytes);

    if (fwrite(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle) != uiBytes)
    {
      plLog::Error("fwrite failed for '{}'", m_sFileName);
      return PL_FAILURE;
    }
  }

  return PL_SUCCESS;
}

plUInt64 plOSFile::InternalRead(void* pBuffer, plUInt64 uiBytes)
{
  plUInt64 uiBytesRead = 0;

  const plUInt32 uiBatchBytes = 1024 * 1024 * 1024; // 1 GB

  // first write out all the data in 1GB batches
  while (uiBytes > uiBatchBytes)
  {
    const plUInt64 uiReadThisTime = fread(pBuffer, 1, uiBatchBytes, m_FileData.m_pFileHandle);
    uiBytesRead += uiReadThisTime;

    if (uiReadThisTime != uiBatchBytes)
      return uiBytesRead;

    uiBytes -= uiBatchBytes;
    pBuffer = plMemoryUtils::AddByteOffset(pBuffer, uiBatchBytes);
  }

  if (uiBytes > 0)
  {
    const plUInt32 uiBytes32 = static_cast<plUInt32>(uiBytes);

    uiBytesRead += fread(pBuffer, 1, uiBytes32, m_FileData.m_pFileHandle);
  }

  return uiBytesRead;
}

plUInt64 plOSFile::InternalGetFilePosition() const
{
#if PL_ENABLED(PL_USE_OLD_POSIX_FUNCTIONS)
  return static_cast<plUInt64>(ftell(m_FileData.m_pFileHandle));
#else
  return static_cast<plUInt64>(ftello(m_FileData.m_pFileHandle));
#endif
}

void plOSFile::InternalSetFilePosition(plInt64 iDistance, plFileSeekMode::Enum Pos) const
{
#if PL_ENABLED(PL_USE_OLD_POSIX_FUNCTIONS)
  switch (Pos)
  {
    case plFileSeekMode::FromStart:
      PL_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case plFileSeekMode::FromEnd:
      PL_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case plFileSeekMode::FromCurrent:
      PL_VERIFY(fseek(m_FileData.m_pFileHandle, (long)iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#else
  switch (Pos)
  {
    case plFileSeekMode::FromStart:
      PL_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_SET) == 0, "Seek Failed");
      break;
    case plFileSeekMode::FromEnd:
      PL_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_END) == 0, "Seek Failed");
      break;
    case plFileSeekMode::FromCurrent:
      PL_VERIFY(fseeko(m_FileData.m_pFileHandle, iDistance, SEEK_CUR) == 0, "Seek Failed");
      break;
  }
#endif
}

// this might not be defined on Windows
#ifndef S_ISDIR
#  define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#endif

bool plOSFile::InternalExistsFile(plStringView sFile)
{
  struct stat sb;
  return (stat(plString(sFile), &sb) == 0 && !S_ISDIR(sb.st_mode));
}

bool plOSFile::InternalExistsDirectory(plStringView sDirectory)
{
  struct stat sb;
  return (stat(plString(sDirectory), &sb) == 0 && S_ISDIR(sb.st_mode));
}

plResult plOSFile::InternalDeleteFile(plStringView sFile)
{
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  int iRes = _unlink(plString(sFile));
#else
  int iRes = unlink(plString(sFile));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plOSFile::InternalDeleteDirectory(plStringView sDirectory)
{
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  int iRes = _rmdir(plString(sDirectory));
#else
  int iRes = rmdir(plString(sDirectory));
#endif

  if (iRes == 0 || (iRes == -1 && errno == ENOENT))
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plOSFile::InternalCreateDirectory(plStringView sDirectory)
{
  // handle drive letters as always successful
  if (plStringUtils::GetCharacterCount(sDirectory.GetStartPointer(), sDirectory.GetEndPointer()) <= 1) // '/'
    return PL_SUCCESS;

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  int iRes = _mkdir(plString(sDirectory));
#else
  int iRes = mkdir(plString(sDirectory), 0777);
#endif

  if (iRes == 0 || (iRes == -1 && errno == EEXIST))
    return PL_SUCCESS;

  // If we were not allowed to access the folder but it alreay exists, we treat the operation as successful.
  // Note that this is espcially relevant for calls to plOSFile::CreateDirectoryStructure where we may call mkdir on top level directories that are
  // not accessible.
  if (errno == EACCES && InternalExistsDirectory(sDirectory))
    return PL_SUCCESS;

  return PL_FAILURE;
}

plResult plOSFile::InternalMoveFileOrDirectory(plStringView sDirectoryFrom, plStringView sDirectoryTo)
{
  if (rename(plString(sDirectoryFrom), plString(sDirectoryTo)) != 0)
  {
    return PL_FAILURE;
  }
  return PL_SUCCESS;
}

#if PL_ENABLED(PL_SUPPORTS_FILE_STATS) && PL_DISABLED(PL_PLATFORM_WINDOWS_UWP)
plResult plOSFile::InternalGetFileStats(plStringView sFileOrFolder, plFileStats& out_Stats)
{
  struct stat tempStat;
  int iRes = stat(plString(sFileOrFolder), &tempStat);

  if (iRes != 0)
    return PL_FAILURE;

  out_Stats.m_bIsDirectory = S_ISDIR(tempStat.st_mode);
  out_Stats.m_uiFileSize = tempStat.st_size;
  out_Stats.m_sParentPath = sFileOrFolder;
  out_Stats.m_sParentPath.PathParentDirectory();
  out_Stats.m_sName = plPathUtils::GetFileNameAndExtension(sFileOrFolder); // no OS support, so just pass it through
  out_Stats.m_LastModificationTime = plTimestamp::MakeFromInt(tempStat.st_mtime, plSIUnitOfTime::Second);

  return PL_SUCCESS;
}
#endif

#if PL_DISABLED(PL_PLATFORM_WINDOWS_UWP)

plStringView plOSFile::GetApplicationDirectory()
{
  static plString256 s_Path;

  if (s_Path.IsEmpty())
  {
#  if PL_ENABLED(PL_PLATFORM_OSX)

    CFBundleRef appBundle = CFBundleGetMainBundle();
    CFURLRef bundleURL = CFBundleCopyBundleURL(appBundle);
    CFStringRef bundlePath = CFURLCopyFileSystemPath(bundleURL, kCFURLPOSIXPathStyle);

    if (bundlePath != nullptr)
    {
      CFIndex length = CFStringGetLength(bundlePath);
      CFIndex maxSize = CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;

      plArrayPtr<char> temp = PL_DEFAULT_NEW_ARRAY(char, static_cast<plUInt32>(maxSize));

      if (CFStringGetCString(bundlePath, temp.GetPtr(), maxSize, kCFStringEncodingUTF8))
      {
        s_Path = temp.GetPtr();
      }

      PL_DEFAULT_DELETE_ARRAY(temp);
    }

    CFRelease(bundlePath);
    CFRelease(bundleURL);
    CFRelease(appBundle);
#  elif PL_ENABLED(PL_PLATFORM_ANDROID)
    {
      plJniAttachment attachment;

      plJniString packagePath = attachment.GetActivity().Call<plJniString>("getPackageCodePath");
      // By convention, android requires assets to be placed in the 'Assets' folder
      // inside the apk thus we use that as our SDK root.
      plStringBuilder sTemp = packagePath.GetData();
      sTemp.AppendPath("Assets");
      s_Path = sTemp;
    }
#  else
    char result[PATH_MAX];
    ssize_t length = readlink("/proc/self/exe", result, PATH_MAX);
    plStringBuilder path(plStringView(result, result + length));
    s_Path = path.GetFileDirectory();
#  endif
  }

  PL_ASSERT_ALWAYS(!s_Path.IsEmpty(), "Failed to retrieve application directory.");

  return s_Path;
}

plString plOSFile::GetUserDataFolder(plStringView sSubFolder)
{
  if (s_sUserDataPath.IsEmpty())
  {
#  if PL_ENABLED(PL_PLATFORM_ANDROID)
    android_app* app = plAndroidUtils::GetAndroidApp();
    s_sUserDataPath = app->activity->internalDataPath;
#  else
    s_sUserDataPath = getenv("HOME");

    if (s_sUserDataPath.IsEmpty())
      s_sUserDataPath = getpwuid(getuid())->pw_dir;
#  endif
  }

  plStringBuilder s = s_sUserDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

plString plOSFile::GetTempDataFolder(plStringView sSubFolder)
{
  if (s_sTempDataPath.IsEmpty())
  {
#  if PL_ENABLED(PL_PLATFORM_ANDROID)
    plJniAttachment attachment;

    plJniObject cacheDir = attachment.GetActivity().Call<plJniObject>("getCacheDir");
    plJniString path = cacheDir.Call<plJniString>("getPath");
    s_sTempDataPath = path.GetData();
#  else
    s_sTempDataPath = GetUserDataFolder(".cache").GetData();
#  endif
  }

  plStringBuilder s = s_sTempDataPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

plString plOSFile::GetUserDocumentsFolder(plStringView sSubFolder)
{
  if (s_sUserDocumentsPath.IsEmpty())
  {
#  if PL_ENABLED(PL_PLATFORM_ANDROID)
    PL_ASSERT_NOT_IMPLEMENTED;
#  else
    s_sUserDataPath = getenv("HOME");

    if (s_sUserDataPath.IsEmpty())
      s_sUserDataPath = getpwuid(getuid())->pw_dir;
#  endif
  }

  plStringBuilder s = s_sUserDocumentsPath;
  s.AppendPath(sSubFolder);
  s.MakeCleanPath();
  return s;
}

const plString plOSFile::GetCurrentWorkingDirectory()
{
  char tmp[PATH_MAX];

  plStringBuilder clean = getcwd(tmp, PL_ARRAY_SIZE(tmp));
  clean.MakeCleanPath();

  return clean;
}

#  if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)

plFileSystemIterator::plFileSystemIterator() = default;

plFileSystemIterator::~plFileSystemIterator()
{
  while (!m_Data.m_Handles.IsEmpty())
  {
    closedir((DIR*)m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();
  }
}

bool plFileSystemIterator::IsValid() const
{
  return !m_Data.m_Handles.IsEmpty();
}

namespace
{
  plResult UpdateCurrentFile(plFileStats& curFile, const plStringBuilder& curPath, DIR* hSearch, const plString& wildcardSearch)
  {
    struct dirent* hCurrentFile = readdir(hSearch);
    if (hCurrentFile == nullptr)
      return PL_FAILURE;

    if (!wildcardSearch.IsEmpty())
    {
      while (fnmatch(wildcardSearch.GetData(), hCurrentFile->d_name, FNM_NOESCAPE) != 0)
      {
        hCurrentFile = readdir(hSearch);
        if (hCurrentFile == nullptr)
          return PL_FAILURE;
      }
    }

    plStringBuilder absFileName = curPath;
    absFileName.AppendPath(hCurrentFile->d_name);

    struct stat fileStat = {};
    stat(absFileName.GetData(), &fileStat);

    curFile.m_uiFileSize = fileStat.st_size;
    curFile.m_bIsDirectory = hCurrentFile->d_type == DT_DIR;
    curFile.m_sParentPath = curPath;
    curFile.m_sName = hCurrentFile->d_name;
    curFile.m_LastModificationTime = plTimestamp::MakeFromInt(fileStat.st_mtime, plSIUnitOfTime::Second);

    return PL_SUCCESS;
  }
} // namespace

void plFileSystemIterator::StartSearch(plStringView sSearchTerm, plBitflags<plFileSystemIteratorFlags> flags /*= plFileSystemIteratorFlags::All*/)
{
  PL_ASSERT_DEV(m_Data.m_Handles.IsEmpty(), "Cannot start another search.");

  m_sSearchTerm = sSearchTerm;

  plStringBuilder sSearch = sSearchTerm;
  sSearch.MakeCleanPath();

  // same as just passing in the folder path, so remove this
  if (sSearch.EndsWith("/*"))
    sSearch.Shrink(0, 2);

  // Remove a trailing slash if any
  sSearch.Trim(nullptr, "/");

  // Since the use of wildcard-ed file names will disable recursion, we ensure both are not used simultaneously.
  const bool bHasWildcard = sSearch.FindLastSubString("*") || sSearch.FindLastSubString("?");
  if (flags.IsSet(plFileSystemIteratorFlags::Recursive) == true && bHasWildcard == true)
  {
    PL_ASSERT_DEV(false, "Recursive file iteration does not support wildcards. Either don't use recursion, or filter the filenames manually.");
    return;
  }

  if (bHasWildcard)
  {
    m_Data.m_wildcardSearch = sSearch.GetFileNameAndExtension();
    m_sCurPath = sSearch.GetFileDirectory();
  }
  else
  {
    m_Data.m_wildcardSearch.Clear();
    m_sCurPath = sSearch;
  }

  PL_ASSERT_DEV(m_sCurPath.IsAbsolutePath(), "The path '{0}' is not absolute.", m_sCurPath);

  m_Flags = flags;

  DIR* hSearch = opendir(m_sCurPath.GetData());

  if (hSearch == nullptr)
    return;

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Failed())
  {
    return;
  }

  m_Data.m_Handles.PushBack(hSearch);

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
  constexpr plInt32 CallInternalNext = 2;

  if (m_Data.m_Handles.IsEmpty())
    return PL_FAILURE;

  if (m_Flags.IsSet(plFileSystemIteratorFlags::Recursive) && m_CurFile.m_bIsDirectory && (m_CurFile.m_sName != "..") && (m_CurFile.m_sName != "."))
  {
    m_sCurPath.AppendPath(m_CurFile.m_sName.GetData());

    DIR* hSearch = opendir(m_sCurPath.GetData());

    if (hSearch != nullptr && UpdateCurrentFile(m_CurFile, m_sCurPath, hSearch, m_Data.m_wildcardSearch).Succeeded())
    {
      m_Data.m_Handles.PushBack(hSearch);

      if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
        return CallInternalNext; // will search for the next file or folder that is not ".." or "." ; might return false though

      if (m_CurFile.m_bIsDirectory)
      {
        if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFolders))
          return CallInternalNext;
      }
      else
      {
        if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFiles))
          return CallInternalNext;
      }

      return PL_SUCCESS;
    }

    // if the recursion did not work, just iterate in this folder further
  }

  if (UpdateCurrentFile(m_CurFile, m_sCurPath, (DIR*)m_Data.m_Handles.PeekBack(), m_Data.m_wildcardSearch).Failed())
  {
    // nothing found in this directory anymore
    closedir((DIR*)m_Data.m_Handles.PeekBack());
    m_Data.m_Handles.PopBack();

    if (m_Data.m_Handles.IsEmpty())
      return PL_FAILURE;

    m_sCurPath.PathParentDirectory();
    if (m_sCurPath.GetElementCount() > 1 && m_sCurPath.EndsWith("/"))
    {
      m_sCurPath.Shrink(0, 1);
    }

    return CallInternalNext;
  }

  if ((m_CurFile.m_sName == "..") || (m_CurFile.m_sName == "."))
    return CallInternalNext;

  if (m_CurFile.m_bIsDirectory)
  {
    if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFolders))
      return CallInternalNext;
  }
  else
  {
    if (!m_Flags.IsSet(plFileSystemIteratorFlags::ReportFiles))
      return CallInternalNext;
  }

  return PL_SUCCESS;
}

#  endif

#endif // PL_DISABLED(PL_PLATFORM_WINDOWS_UWP)
