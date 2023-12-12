#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/OSFile.h>

void plQtEditorApp::AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName, bool bWriteable)
{
  plStringBuilder sPath = szSdkRootRelativePath;
  sPath.MakeCleanPath();

  for (auto& dd : m_FileSystemConfig.m_DataDirs)
  {
    if (dd.m_sDataDirSpecialPath == sPath)
    {
      dd.m_bHardCodedDependency = true;

      if (bWriteable)
        dd.m_bWritable = true;

      return;
    }
  }

  plApplicationFileSystemConfig::DataDirConfig cfg;
  cfg.m_sDataDirSpecialPath = sPath;
  cfg.m_bWritable = bWriteable;
  cfg.m_sRootName = szRootName;
  cfg.m_bHardCodedDependency = true;

  m_FileSystemConfig.m_DataDirs.PushBack(cfg);
}

void plQtEditorApp::SetFileSystemConfig(const plApplicationFileSystemConfig& cfg)
{
  if (m_FileSystemConfig == cfg)
    return;

  m_FileSystemConfig = cfg;
  plQtEditorApp::GetSingleton()->AddReloadProjectRequiredReason("The data directory configuration has changed.");

  m_FileSystemConfig.CreateDataDirStubFiles().IgnoreResult();
}

void plQtEditorApp::SetupDataDirectories()
{
  PLASMA_PROFILE_SCOPE("SetupDataDirectories");
  plFileSystem::DetectSdkRootDirectory().IgnoreResult();

  plStringBuilder sPath = plToolsProject::GetSingleton()->GetProjectDirectory();

  plFileSystem::SetSpecialDirectory("project", sPath);

  sPath.AppendPath("RuntimeConfigs/DataDirectories.ddl");
  // we cannot use the default ":project/" path here, because that data directory will only be configured a few lines below
  // so instead we use the absolute path directly
  m_FileSystemConfig.Load(sPath);

  PlasmaEditorAppEvent e;
  e.m_Type = PlasmaEditorAppEvent::Type::BeforeApplyDataDirectories;
  m_Events.Broadcast(e);

  plQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Base", "base", false);
  plQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">sdk/Data/Plugins", "plugins", false);
  plQtEditorApp::GetSingleton()->AddPluginDataDirDependency(">project/", "project", true);

  // Tell the tools project that all data directories are ok to put documents in
  {
    for (const auto& dd : m_FileSystemConfig.m_DataDirs)
    {
      if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sPath).Succeeded())
      {
        plToolsProject::GetSingleton()->AddAllowedDocumentRoot(sPath);
      }
    }
  }

  m_FileSystemConfig.Apply();
}

bool plQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute(plStringBuilder& sPath, bool bCheckExists) const
{
  sPath.MakeCleanPath();

  if (plPathUtils::IsAbsolutePath(sPath))
    return true;

  if (plPathUtils::IsRootedPath(sPath))
  {
    plStringBuilder sAbsPath;
    if (plFileSystem::ResolvePath(sPath, &sAbsPath, nullptr).Succeeded())
    {
      sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (plConversionUtils::IsStringUuid(sPath))
  {
    plUuid guid = plConversionUtils::ConvertStringToUuid(sPath);
    auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    sPath = pAsset->m_pAssetInfo->m_sAbsolutePath;
    return true;
  }

  plStringBuilder sTemp, sFolder, sDataDirName;

  const char* szEnd = sPath.FindSubString("/");
  if (szEnd)
  {
    sDataDirName.SetSubString_FromTo(sPath.GetData(), szEnd);
  }
  else
  {
    sDataDirName = sPath;
  }

  for (plUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    // only check data directories that start with the required name
    while (sTemp.EndsWith("/") || sTemp.EndsWith("\\"))
      sTemp.Shrink(0, 1);
    const plStringView folderName = sTemp.GetFileName();

    if (sDataDirName != folderName)
      continue;

    sTemp.PathParentDirectory(); // the secret sauce is here
    sTemp.AppendPath(sPath);
    sTemp.MakeCleanPath();

    if (!bCheckExists || plOSFile::ExistsFile(sTemp) || plOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool plQtEditorApp::MakeDataDirectoryRelativePathAbsolute(plStringBuilder& sPath) const
{
  if (plPathUtils::IsAbsolutePath(sPath))
    return true;

  if (plPathUtils::IsRootedPath(sPath))
  {
    plStringBuilder sAbsPath;
    if (plFileSystem::ResolvePath(sPath, &sAbsPath, nullptr).Succeeded())
    {
      sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (plConversionUtils::IsStringUuid(sPath))
  {
    plUuid guid = plConversionUtils::ConvertStringToUuid(sPath);
    auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    sPath = pAsset->m_pAssetInfo->m_sAbsolutePath;
    return true;
  }

  plStringBuilder sTemp;

  for (plUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.AppendPath(sPath);
    sTemp.MakeCleanPath();

    if (plOSFile::ExistsFile(sTemp) || plOSFile::ExistsDirectory(sTemp))
    {
      sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool plQtEditorApp::MakeDataDirectoryRelativePathAbsolute(plString& sPath) const
{
  plStringBuilder sTemp = sPath;
  bool bRes = MakeDataDirectoryRelativePathAbsolute(sTemp);
  sPath = sTemp;
  return bRes;
}

bool plQtEditorApp::MakePathDataDirectoryRelative(plStringBuilder& sPath) const
{
  plStringBuilder sTemp;

  for (plUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (sPath.IsPathBelowFolder(sTemp))
    {
      sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  sPath.MakeRelativeTo(plFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool plQtEditorApp::MakePathDataDirectoryParentRelative(plStringBuilder& sPath) const
{
  plStringBuilder sTemp;

  for (plUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (sPath.IsPathBelowFolder(sTemp))
    {
      sTemp.PathParentDirectory();

      sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  sPath.MakeRelativeTo(plFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool plQtEditorApp::MakePathDataDirectoryRelative(plString& sPath) const
{
  plStringBuilder sTemp = sPath;
  bool bRes = MakePathDataDirectoryRelative(sTemp);
  sPath = sTemp;
  return bRes;
}
