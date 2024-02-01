#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/Implementation/StringIterator.h>
#include <Foundation/Strings/StringView.h>

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FileSystem)

  ON_CORESYSTEMS_STARTUP
  {
    plFileSystem::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plFileSystem::Shutdown();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plFileSystem::FileSystemData* plFileSystem::s_pData = nullptr;
plString plFileSystem::s_sSdkRootDir;
plMap<plString, plString> plFileSystem::s_SpecialDirectories;


void plFileSystem::RegisterDataDirectoryFactory(plDataDirFactory factory, float fPriority /*= 0*/)
{
  PL_LOCK(s_pData->m_FsMutex);

  auto& data = s_pData->m_DataDirFactories.ExpandAndGetRef();
  data.m_Factory = factory;
  data.m_fPriority = fPriority;
}

plEventSubscriptionID plFileSystem::RegisterEventHandler(plEvent<const FileEvent&>::Handler handler)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_Event.AddEventHandler(handler);
}

void plFileSystem::UnregisterEventHandler(plEvent<const FileEvent&>::Handler handler)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(handler);
}

void plFileSystem::UnregisterEventHandler(plEventSubscriptionID subscriptionId)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  s_pData->m_Event.RemoveEventHandler(subscriptionId);
}

void plFileSystem::CleanUpRootName(plStringBuilder& sRoot)
{
  // this cleaning might actually make the root name empty
  // e.g. ":" becomes ""
  // which is intended to support passing through of absolute paths
  // ie. mounting the empty dir "" under the root ":" will allow to write directly to files using absolute paths

  while (sRoot.StartsWith(":"))
    sRoot.Shrink(1, 0);

  while (sRoot.EndsWith("/"))
    sRoot.Shrink(0, 1);

  sRoot.ToUpper();
}

plResult plFileSystem::AddDataDirectory(plStringView sDataDirectory, plStringView sGroup, plStringView sRootName, DataDirUsage usage)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  PL_ASSERT_DEV(usage != AllowWrites || !sRootName.IsEmpty(), "A data directory must have a non-empty, unique name to be mounted for write access");

  plStringBuilder sPath = sDataDirectory;
  sPath.MakeCleanPath();

  if (!sPath.IsEmpty() && !sPath.EndsWith("/"))
    sPath.Append("/");

  plStringBuilder sCleanRootName = sRootName;
  CleanUpRootName(sCleanRootName);

  PL_LOCK(s_pData->m_FsMutex);

  bool failed = false;
  if (FindDataDirectoryWithRoot(sCleanRootName) != nullptr)
  {
    plLog::Error("A data directory with root name '{0}' already exists.", sCleanRootName);
    failed = true;
  }

  if (!failed)
  {
    s_pData->m_DataDirFactories.Sort([](const auto& a, const auto& b) { return a.m_fPriority < b.m_fPriority; });

    // use the factory that was added last as the one with the highest priority -> allows to override already added factories
    for (plInt32 i = s_pData->m_DataDirFactories.GetCount() - 1; i >= 0; --i)
    {
      plDataDirectoryType* pDataDir = s_pData->m_DataDirFactories[i].m_Factory(sPath, sGroup, sRootName, usage);

      if (pDataDir != nullptr)
      {
        DataDirectory dd;
        dd.m_Usage = usage;
        dd.m_pDataDirectory = pDataDir;
        dd.m_sRootName = sCleanRootName;
        dd.m_sGroup = sGroup;

        s_pData->m_DataDirectories.PushBack(dd);

        {
          // Broadcast that a data directory was added
          FileEvent fe;
          fe.m_EventType = FileEventType::AddDataDirectorySucceeded;
          fe.m_sFileOrDirectory = sPath;
          fe.m_sOther = sCleanRootName;
          fe.m_pDataDir = pDataDir;
          s_pData->m_Event.Broadcast(fe);
        }

        return PL_SUCCESS;
      }
    }
  }

  {
    // Broadcast that adding a data directory failed
    FileEvent fe;
    fe.m_EventType = FileEventType::AddDataDirectoryFailed;
    fe.m_sFileOrDirectory = sPath;
    fe.m_sOther = sCleanRootName;
    s_pData->m_Event.Broadcast(fe);
  }

  plLog::Error("Adding Data Directory '{0}' failed.", plArgSensitive(sDataDirectory, "Path"));
  return PL_FAILURE;
}


bool plFileSystem::RemoveDataDirectory(plStringView sRootName)
{
  plStringBuilder sCleanRootName = sRootName;
  CleanUpRootName(sCleanRootName);

  PL_LOCK(s_pData->m_FsMutex);

  for (plUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    const auto& directory = s_pData->m_DataDirectories[i];

    if (directory.m_sRootName == sCleanRootName)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = directory.m_pDataDirectory->GetDataDirectoryPath();
        fe.m_sOther = directory.m_sRootName;
        fe.m_pDataDir = directory.m_pDataDirectory;
        s_pData->m_Event.Broadcast(fe);
      }

      directory.m_pDataDirectory->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);

      return true;
    }
    else
      ++i;
  }

  return false;
}

plUInt32 plFileSystem::RemoveDataDirectoryGroup(plStringView sGroup)
{
  if (s_pData == nullptr)
    return 0;

  PL_LOCK(s_pData->m_FsMutex);

  plUInt32 uiRemoved = 0;

  for (plUInt32 i = 0; i < s_pData->m_DataDirectories.GetCount();)
  {
    if (s_pData->m_DataDirectories[i].m_sGroup == sGroup)
    {
      {
        // Broadcast that a data directory is about to be removed
        FileEvent fe;
        fe.m_EventType = FileEventType::RemoveDataDirectory;
        fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirectory->GetDataDirectoryPath();
        fe.m_sOther = s_pData->m_DataDirectories[i].m_sRootName;
        fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirectory;
        s_pData->m_Event.Broadcast(fe);
      }

      ++uiRemoved;

      s_pData->m_DataDirectories[i].m_pDataDirectory->RemoveDataDirectory();
      s_pData->m_DataDirectories.RemoveAtAndCopy(i);
    }
    else
      ++i;
  }

  return uiRemoved;
}

void plFileSystem::ClearAllDataDirectories()
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  PL_LOCK(s_pData->m_FsMutex);

  for (plInt32 i = s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    {
      // Broadcast that a data directory is about to be removed
      FileEvent fe;
      fe.m_EventType = FileEventType::RemoveDataDirectory;
      fe.m_sFileOrDirectory = s_pData->m_DataDirectories[i].m_pDataDirectory->GetDataDirectoryPath();
      fe.m_sOther = s_pData->m_DataDirectories[i].m_sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirectory->RemoveDataDirectory();
  }

  s_pData->m_DataDirectories.Clear();
}

plDataDirectoryType* plFileSystem::FindDataDirectoryWithRoot(plStringView sRootName)
{
  if (sRootName.IsEmpty())
    return nullptr;

  PL_LOCK(s_pData->m_FsMutex);

  for (const auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_sRootName.IsEqual_NoCase(sRootName))
    {
      return dd.m_pDataDirectory;
    }
  }

  return nullptr;
}

plUInt32 plFileSystem::GetNumDataDirectories()
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories.GetCount();
}

plDataDirectoryType* plFileSystem::GetDataDirectory(plUInt32 uiDataDirIndex)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  return s_pData->m_DataDirectories[uiDataDirIndex].m_pDataDirectory;
}

plStringView plFileSystem::GetDataDirRelativePath(plStringView sPath, plUInt32 uiDataDir)
{
  PL_LOCK(s_pData->m_FsMutex);

  // if an absolute path is given, this will check whether the absolute path would fall into this data directory
  // if yes, the prefix path is removed and then only the relative path is given to the data directory type
  // otherwise the data directory would prepend its own path and thus create an invalid path to work with

  // first check the redirected directory
  const plString128& sRedDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirectory->GetRedirectedDataDirectoryPath();

  if (!sRedDirPath.IsEmpty() && sPath.StartsWith_NoCase(sRedDirPath))
  {
    plStringView sRelPath(sPath.GetStartPointer() + sRedDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (plPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  // then check the original mount path
  const plString128& sDirPath = s_pData->m_DataDirectories[uiDataDir].m_pDataDirectory->GetDataDirectoryPath();

  // If the data dir is empty we return the paths as is or the code below would remove the '/' in front of an
  // absolute path.
  if (!sDirPath.IsEmpty() && sPath.StartsWith_NoCase(sDirPath))
  {
    plStringView sRelPath(sPath.GetStartPointer() + sDirPath.GetElementCount(), sPath.GetEndPointer());

    // if the relative path still starts with a path-separator, skip it
    if (plPathUtils::IsPathSeparator(sRelPath.GetCharacter()))
    {
      sRelPath.ChopAwayFirstCharacterUtf8();
    }

    return sRelPath;
  }

  return sPath;
}


plFileSystem::DataDirectory* plFileSystem::GetDataDirForRoot(const plString& sRoot)
{
  PL_LOCK(s_pData->m_FsMutex);

  for (plInt32 i = (plInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_sRootName == sRoot)
      return &s_pData->m_DataDirectories[i];
  }

  return nullptr;
}


void plFileSystem::DeleteFile(plStringView sFile)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (plPathUtils::IsAbsolutePath(sFile))
  {
    plOSFile::DeleteFile(sFile).IgnoreResult();
    return;
  }

  plString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  PL_ASSERT_DEV(!sRootName.IsEmpty(), "Files can only be deleted with a rooted path name.");

  if (sRootName.IsEmpty())
    return;

  PL_LOCK(s_pData->m_FsMutex);

  for (plInt32 i = (plInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // do not delete data from directories that are mounted as read only
    if (s_pData->m_DataDirectories[i].m_Usage != AllowWrites)
      continue;

    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    plStringView sRelPath = GetDataDirRelativePath(sFile, i);

    {
      // Broadcast that a file is about to be deleted
      // This can be used to check out files or mark them as deleted in a revision control system
      FileEvent fe;
      fe.m_EventType = FileEventType::DeleteFile;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirectory;
      fe.m_sOther = sRootName;
      s_pData->m_Event.Broadcast(fe);
    }

    s_pData->m_DataDirectories[i].m_pDataDirectory->DeleteFile(sRelPath);
  }
}

bool plFileSystem::ExistsFile(plStringView sFile)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  plString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  PL_LOCK(s_pData->m_FsMutex);

  for (plInt32 i = (plInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    plStringView sRelPath = GetDataDirRelativePath(sFile, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirectory->ExistsFile(sRelPath, bOneSpecificDataDir))
      return true;
  }

  return false;
}


plResult plFileSystem::GetFileStats(plStringView sFileOrFolder, plFileStats& out_stats)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  PL_LOCK(s_pData->m_FsMutex);

  plString sRootName;
  sFileOrFolder = ExtractRootName(sFileOrFolder, sRootName);

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  for (plInt32 i = (plInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (!sRootName.IsEmpty() && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    plStringView sRelPath = GetDataDirRelativePath(sFileOrFolder, i);

    if (s_pData->m_DataDirectories[i].m_pDataDirectory->GetFileStats(sRelPath, bOneSpecificDataDir, out_stats).Succeeded())
      return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plStringView plFileSystem::ExtractRootName(plStringView sPath, plString& rootName)
{
  rootName.Clear();

  if (!sPath.StartsWith(":"))
    return sPath;

  plStringBuilder sCur;
  const plStringView view = sPath;
  plStringIterator it = view.GetIteratorFront();
  ++it;

  while (it.IsValid() && (it.GetCharacter() != '/'))
  {
    sCur.Append(it.GetCharacter());
    ++it;
  }

  PL_ASSERT_DEV(it.IsValid(), "Cannot parse the path \"{0}\". The data-dir root name starts with a ':' but does not end with '/'.", sPath);

  sCur.ToUpper();
  rootName = sCur;
  ++it;

  return it.GetData(); // return the string after the data-dir filter declaration
}

plDataDirectoryReader* plFileSystem::GetFileReader(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  PL_LOCK(s_pData->m_FsMutex);

  plString sRootName;
  sFile = ExtractRootName(sFile, sRootName);

  // clean up the path to get rid of ".." etc.
  plStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  const bool bOneSpecificDataDir = !sRootName.IsEmpty();

  // the last added data directory has the highest priority
  for (plInt32 i = (plInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    // if a root is used, ignore all directories that do not have the same root name
    if (bOneSpecificDataDir && s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    plStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType = FileEventType::OpenFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);
    }

    // Let the data directory try to open the file.
    plDataDirectoryReader* pReader = s_pData->m_DataDirectories[i].m_pDataDirectory->OpenFileToRead(sRelPath, FileShareMode, bOneSpecificDataDir);

    if (bAllowFileEvents && pReader != nullptr)
    {
      // Broadcast that this file has been opened.
      FileEvent fe;
      fe.m_EventType = FileEventType::OpenFileSucceeded;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);

      return pReader;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that opening this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::OpenFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

plDataDirectoryWriter* plFileSystem::GetFileWriter(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bAllowFileEvents)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  if (sFile.IsEmpty())
    return nullptr;

  PL_LOCK(s_pData->m_FsMutex);

  plString sRootName;

  if (!plPathUtils::IsAbsolutePath(sFile))
  {
    PL_ASSERT_DEV(sFile.StartsWith(":"),
      "Only native absolute paths or rooted paths (starting with a colon and then the data dir root name) are allowed for "
      "writing to files. This path is neither: '{0}'",
      sFile);
    sFile = ExtractRootName(sFile, sRootName);
  }

  // clean up the path to get rid of ".." etc.
  plStringBuilder sPath = sFile;
  sPath.MakeCleanPath();

  // the last added data directory has the highest priority
  for (plInt32 i = (plInt32)s_pData->m_DataDirectories.GetCount() - 1; i >= 0; --i)
  {
    if (s_pData->m_DataDirectories[i].m_Usage != AllowWrites)
      continue;

    // ignore all directories that have not the category that is currently requested
    if (s_pData->m_DataDirectories[i].m_sRootName != sRootName)
      continue;

    plStringView sRelPath = GetDataDirRelativePath(sPath, i);

    if (bAllowFileEvents)
    {
      // Broadcast that we now try to open this file
      // Could be useful to check this file out before it is accessed
      FileEvent fe;
      fe.m_EventType = FileEventType::CreateFileAttempt;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);
    }

    plDataDirectoryWriter* pWriter = s_pData->m_DataDirectories[i].m_pDataDirectory->OpenFileToWrite(sRelPath, FileShareMode);

    if (bAllowFileEvents && pWriter != nullptr)
    {
      // Broadcast that this file has been created.
      FileEvent fe;
      fe.m_EventType = FileEventType::CreateFileSucceeded;
      fe.m_sFileOrDirectory = sRelPath;
      fe.m_sOther = sRootName;
      fe.m_pDataDir = s_pData->m_DataDirectories[i].m_pDataDirectory;
      s_pData->m_Event.Broadcast(fe);

      return pWriter;
    }
  }

  if (bAllowFileEvents)
  {
    // Broadcast that creating this file failed.
    FileEvent fe;
    fe.m_EventType = FileEventType::CreateFileFailed;
    fe.m_sFileOrDirectory = sPath;
    s_pData->m_Event.Broadcast(fe);
  }

  return nullptr;
}

plResult plFileSystem::ResolvePath(plStringView sPath, plStringBuilder* out_pAbsolutePath, plStringBuilder* out_pDataDirRelativePath, plDataDirectoryType** out_pDataDir /*= nullptr*/)
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");

  PL_LOCK(s_pData->m_FsMutex);

  plStringBuilder absPath, relPath;

  if (sPath.StartsWith(":"))
  {
    // writing is only allowed using rooted paths
    plString sRootName;
    ExtractRootName(sPath, sRootName);

    DataDirectory* pDataDir = GetDataDirForRoot(sRootName);

    if (pDataDir == nullptr)
      return PL_FAILURE;

    if (out_pDataDir != nullptr)
      *out_pDataDir = pDataDir->m_pDataDirectory;

    relPath = sPath.GetShrunk(sRootName.GetCharacterCount() + 2);

    absPath = pDataDir->m_pDataDirectory->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);
  }
  else if (plPathUtils::IsAbsolutePath(sPath))
  {
    absPath = sPath;
    absPath.MakeCleanPath();

    for (plUInt32 dd = s_pData->m_DataDirectories.GetCount(); dd > 0; --dd)
    {
      auto& dir = s_pData->m_DataDirectories[dd - 1];

      if (plPathUtils::IsSubPath(dir.m_pDataDirectory->GetRedirectedDataDirectoryPath(), absPath))
      {
        if (out_pAbsolutePath)
          *out_pAbsolutePath = absPath;

        if (out_pDataDirRelativePath)
        {
          *out_pDataDirRelativePath = absPath;
          out_pDataDirRelativePath->MakeRelativeTo(dir.m_pDataDirectory->GetRedirectedDataDirectoryPath()).IgnoreResult();
        }

        if (out_pDataDir)
          *out_pDataDir = dir.m_pDataDirectory;

        return PL_SUCCESS;
      }
    }

    return PL_FAILURE;
  }
  else
  {
    // try to get a reader -> if we get one, the file does indeed exist
    plDataDirectoryReader* pReader = plFileSystem::GetFileReader(sPath, plFileShareMode::SharedReads, true);

    if (!pReader)
      return PL_FAILURE;

    if (out_pDataDir != nullptr)
      *out_pDataDir = pReader->GetDataDirectory();

    relPath = pReader->GetFilePath();

    absPath = pReader->GetDataDirectory()->GetRedirectedDataDirectoryPath(); /// \todo We might also need the none-redirected path as an output
    absPath.AppendPath(relPath);

    pReader->Close();
  }

  if (out_pAbsolutePath)
    *out_pAbsolutePath = absPath;

  if (out_pDataDirRelativePath)
    *out_pDataDirRelativePath = relPath;

  return PL_SUCCESS;
}

plResult plFileSystem::FindFolderWithSubPath(plStringBuilder& out_sResult, plStringView sStartDirectory, plStringView sSubPath, plStringView sRedirectionFileName /*= nullptr*/)
{
  plStringBuilder sStartDirAbs = sStartDirectory;
  sStartDirAbs.MakeCleanPath();

  // in this case the given path and the absolute path are different
  // but we want to return the same path format as is given
  // ie. if we get ":MyRoot\Bla" with "MyRoot" pointing to "C:\Game", then the result should be
  // ":MyRoot\blub", rather than "C:\Game\blub"
  if (sStartDirAbs.StartsWith(":"))
  {
    plStringBuilder abs;
    if (ResolvePath(sStartDirAbs, &abs, nullptr).Failed())
    {
      out_sResult.Clear();
      return PL_FAILURE;
    }

    sStartDirAbs = abs;
  }

  out_sResult = sStartDirectory;
  out_sResult.MakeCleanPath();

  plStringBuilder FullPath, sRedirection;

  while (!out_sResult.IsEmpty())
  {
    sRedirection.Clear();

    if (!sRedirectionFileName.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirectionFileName);

      plOSFile f;
      if (f.Open(FullPath, plFileOpenMode::Read).Succeeded())
      {
        plDataBuffer db;
        f.ReadAll(db);
        sRedirection.Set(plStringView((const char*)db.GetData(), db.GetCount()));
      }
    }

    // first try with the redirection
    if (!sRedirection.IsEmpty())
    {
      FullPath = sStartDirAbs;
      FullPath.AppendPath(sRedirection);
      FullPath.AppendPath(sSubPath);
      FullPath.MakeCleanPath();

      if (plOSFile::ExistsDirectory(FullPath) || plOSFile::ExistsFile(FullPath))
      {
        out_sResult.AppendPath(sRedirection);
        out_sResult.MakeCleanPath();
        return PL_SUCCESS;
      }
    }

    // then try without the redirection
    FullPath = sStartDirAbs;
    FullPath.AppendPath(sSubPath);
    FullPath.MakeCleanPath();

    if (plOSFile::ExistsDirectory(FullPath) || plOSFile::ExistsFile(FullPath))
    {
      return PL_SUCCESS;
    }

    out_sResult.PathParentDirectory();
    sStartDirAbs.PathParentDirectory();
  }

  return PL_FAILURE;
}

bool plFileSystem::ResolveAssetRedirection(plStringView sPathOrAssetGuid, plStringBuilder& out_sRedirection)
{
  PL_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    if (dd.m_pDataDirectory->ResolveAssetRedirection(sPathOrAssetGuid, out_sRedirection))
      return true;
  }

  out_sRedirection = sPathOrAssetGuid;
  return false;
}

plStringView plFileSystem::MigrateFileLocation(plStringView sOldLocation, plStringView sNewLocation)
{
  plStringBuilder sOldPathFull, sNewPathFull;

  if (ResolvePath(sOldLocation, &sOldPathFull, nullptr).Failed() || sOldPathFull.IsEmpty())
  {
    // if the old path could not be resolved, use the new path
    return sNewLocation;
  }

  ResolvePath(sNewLocation, &sNewPathFull, nullptr).AssertSuccess();

  if (!ExistsFile(sOldPathFull))
  {
    // old path doesn't exist -> use the new
    return sNewLocation;
  }

  // old path does exist -> deal with it

  if (ExistsFile(sNewPathFull))
  {
    // new path also exists -> delete the old one (in all data directories), use the new one
    DeleteFile(sOldLocation); // location, not full path
    return sNewLocation;
  }

  // new one doesn't exist -> try to move old to new
  if (plOSFile::MoveFileOrDirectory(sOldPathFull, sNewPathFull).Failed())
  {
    // if the old location exists, but we can't move the file, return the old location to use
    return sOldLocation;
  }

  // deletes the file in the old location in ALL data directories,
  // so that they can't interfere with the new file in the future
  DeleteFile(sOldLocation); // location, not full path

  // if we successfully moved the file to the new location, use the new location
  return sNewLocation;
}

void plFileSystem::ReloadAllExternalDataDirectoryConfigs()
{
  PL_LOG_BLOCK("ReloadAllExternalDataDirectoryConfigs");

  PL_LOCK(s_pData->m_FsMutex);

  for (auto& dd : s_pData->m_DataDirectories)
  {
    dd.m_pDataDirectory->ReloadExternalConfigs();
  }
}

void plFileSystem::Startup()
{
  s_pData = PL_DEFAULT_NEW(FileSystemData);
}

void plFileSystem::Shutdown()
{
  {
    PL_LOCK(s_pData->m_FsMutex);

    s_pData->m_DataDirFactories.Clear();

    ClearAllDataDirectories();
  }

  PL_DEFAULT_DELETE(s_pData);
}

plResult plFileSystem::DetectSdkRootDirectory(plStringView sExpectedSubFolder /*= "Data/Base"*/)
{
  if (!s_sSdkRootDir.IsEmpty())
    return PL_SUCCESS;

  plStringBuilder sdkRoot;

#if PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  // Probably this is what needs to be done on all mobile platforms as well
  sdkRoot = plOSFile::GetApplicationDirectory();
#elif PL_ENABLED(PL_PLATFORM_ANDROID)
  sdkRoot = plOSFile::GetApplicationDirectory();
#else
  if (plFileSystem::FindFolderWithSubPath(sdkRoot, plOSFile::GetApplicationDirectory(), sExpectedSubFolder, "plSdkRoot.txt").Failed())
  {
    plLog::Error("Could not find SDK root. Application dir is '{0}'. Searched for parent with '{1}' sub-folder.", plOSFile::GetApplicationDirectory(), sExpectedSubFolder);
    return PL_FAILURE;
  }
#endif

  plFileSystem::SetSdkRootDirectory(sdkRoot);
  return PL_SUCCESS;
}

void plFileSystem::SetSdkRootDirectory(plStringView sSdkDir)
{
  plStringBuilder s = sSdkDir;
  s.MakeCleanPath();

  s_sSdkRootDir = s;
}

plStringView plFileSystem::GetSdkRootDirectory()
{
  PL_ASSERT_DEV(!s_sSdkRootDir.IsEmpty(), "The project directory has not been set through 'plFileSystem::SetSdkRootDirectory'.");
  return s_sSdkRootDir;
}

void plFileSystem::SetSpecialDirectory(plStringView sName, plStringView sReplacement)
{
  plStringBuilder tmp = sName;
  tmp.ToLower();

  if (sReplacement.IsEmpty())
  {
    s_SpecialDirectories.Remove(tmp);
  }
  else
  {
    s_SpecialDirectories[tmp] = sReplacement;
  }
}

plResult plFileSystem::ResolveSpecialDirectory(plStringView sDirectory, plStringBuilder& out_sPath)
{
  if (sDirectory.IsEmpty() || !sDirectory.StartsWith(">"))
  {
    out_sPath = sDirectory;
    return PL_SUCCESS;
  }

  // skip the '>'
  sDirectory.ChopAwayFirstCharacterAscii();
  const char* szStart = sDirectory.GetStartPointer();

  const char* szEnd = sDirectory.FindSubString("/");

  if (szEnd == nullptr)
    szEnd = szStart + plStringUtils::GetStringElementCount(szStart);

  plStringBuilder sName;
  sName.SetSubString_FromTo(szStart, szEnd);
  sName.ToLower();

  const auto it = s_SpecialDirectories.Find(sName);
  if (it.IsValid())
  {
    out_sPath = it.Value();
    out_sPath.AppendPath(szEnd); // szEnd might be on \0 or a slash
    out_sPath.MakeCleanPath();
    return PL_SUCCESS;
  }

  if (sName == "sdk")
  {
    sDirectory.Shrink(3, 0);
    out_sPath = GetSdkRootDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return PL_SUCCESS;
  }

  if (sName == "user")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = plOSFile::GetUserDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return PL_SUCCESS;
  }

  if (sName == "temp")
  {
    sDirectory.Shrink(4, 0);
    out_sPath = plOSFile::GetTempDataFolder();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return PL_SUCCESS;
  }

  if (sName == "appdir")
  {
    sDirectory.Shrink(6, 0);
    out_sPath = plOSFile::GetApplicationDirectory();
    out_sPath.AppendPath(sDirectory);
    out_sPath.MakeCleanPath();
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}


plMutex& plFileSystem::GetMutex()
{
  PL_ASSERT_DEV(s_pData != nullptr, "FileSystem is not initialized.");
  return s_pData->m_FsMutex;
}

#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS)

void plFileSystem::StartSearch(plFileSystemIterator& ref_iterator, plStringView sSearchTerm, plBitflags<plFileSystemIteratorFlags> flags /*= plFileSystemIteratorFlags::Default*/)
{
  PL_LOCK(s_pData->m_FsMutex);

  plHybridArray<plString, 16> folders;
  plStringBuilder sDdPath;

  for (const auto& dd : s_pData->m_DataDirectories)
  {
    sDdPath = dd.m_pDataDirectory->GetRedirectedDataDirectoryPath();

    if (ResolvePath(sDdPath, &sDdPath, nullptr).Failed())
      continue;

    if (sDdPath.IsEmpty() || !plOSFile::ExistsDirectory(sDdPath))
      continue;


    folders.PushBack(sDdPath);
  }

  ref_iterator.StartMultiFolderSearch(folders, sSearchTerm, flags);
}

#endif

plResult plFileSystem::CreateDirectoryStructure(plStringView sPath)
{
  plStringBuilder sRedir;
  PL_SUCCEED_OR_RETURN(ResolveSpecialDirectory(sPath, sRedir));

  return plOSFile::CreateDirectoryStructure(sRedir);
}

PL_STATICLINK_FILE(Foundation, Foundation_IO_FileSystem_Implementation_FileSystem);
