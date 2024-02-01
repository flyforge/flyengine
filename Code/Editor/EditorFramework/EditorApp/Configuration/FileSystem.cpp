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
  PL_PROFILE_SCOPE("SetupDataDirectories");
  plFileSystem::DetectSdkRootDirectory().IgnoreResult();

  plStringBuilder sPath = plToolsProject::GetSingleton()->GetProjectDirectory();

  plFileSystem::SetSpecialDirectory("project", sPath);

  sPath.AppendPath("RuntimeConfigs/DataDirectories.ddl");
  // we cannot use the default ":project/" path here, because that data directory will only be configured a few lines below
  // so instead we use the absolute path directly
  m_FileSystemConfig.Load(sPath);

  plEditorAppEvent e;
  e.m_Type = plEditorAppEvent::Type::BeforeApplyDataDirectories;
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

bool plQtEditorApp::MakeParentDataDirectoryRelativePathAbsolute(plStringBuilder& ref_sPath, bool bCheckExists) const
{
  ref_sPath.MakeCleanPath();

  if (plPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (plPathUtils::IsRootedPath(ref_sPath))
  {
    plStringBuilder sAbsPath;
    if (plFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (plConversionUtils::IsStringUuid(ref_sPath))
  {
    plUuid guid = plConversionUtils::ConvertStringToUuid(ref_sPath);
    auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  plStringBuilder sTemp, sFolder, sDataDirName;

  const char* szEnd = ref_sPath.FindSubString("/");
  if (szEnd)
  {
    sDataDirName.SetSubString_FromTo(ref_sPath.GetData(), szEnd);
  }
  else
  {
    sDataDirName = ref_sPath;
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
    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (!bCheckExists || plOSFile::ExistsFile(sTemp) || plOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool plQtEditorApp::MakeDataDirectoryRelativePathAbsolute(plStringBuilder& ref_sPath) const
{
  if (plPathUtils::IsAbsolutePath(ref_sPath))
    return true;

  if (plPathUtils::IsRootedPath(ref_sPath))
  {
    plStringBuilder sAbsPath;
    if (plFileSystem::ResolvePath(ref_sPath, &sAbsPath, nullptr).Succeeded())
    {
      ref_sPath = sAbsPath;
      return true;
    }

    return false;
  }

  if (plConversionUtils::IsStringUuid(ref_sPath))
  {
    plUuid guid = plConversionUtils::ConvertStringToUuid(ref_sPath);
    auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(guid);

    if (pAsset == nullptr)
      return false;

    ref_sPath = pAsset->m_pAssetInfo->m_Path;
    return true;
  }

  plStringBuilder sTemp;

  for (plUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    sTemp.AppendPath(ref_sPath);
    sTemp.MakeCleanPath();

    if (plOSFile::ExistsFile(sTemp) || plOSFile::ExistsDirectory(sTemp))
    {
      ref_sPath = sTemp;
      return true;
    }
  }

  return false;
}

bool plQtEditorApp::MakeDataDirectoryRelativePathAbsolute(plString& ref_sPath) const
{
  plStringBuilder sTemp = ref_sPath;
  bool bRes = MakeDataDirectoryRelativePathAbsolute(sTemp);
  ref_sPath = sTemp;
  return bRes;
}

bool plQtEditorApp::MakePathDataDirectoryRelative(plStringBuilder& ref_sPath) const
{
  plStringBuilder sTemp;

  for (plUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(plFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool plQtEditorApp::MakePathDataDirectoryParentRelative(plStringBuilder& ref_sPath) const
{
  plStringBuilder sTemp;

  for (plUInt32 i = m_FileSystemConfig.m_DataDirs.GetCount(); i > 0; --i)
  {
    const auto& dd = m_FileSystemConfig.m_DataDirs[i - 1];

    if (plFileSystem::ResolveSpecialDirectory(dd.m_sDataDirSpecialPath, sTemp).Failed())
      continue;

    if (ref_sPath.IsPathBelowFolder(sTemp))
    {
      sTemp.PathParentDirectory();

      ref_sPath.MakeRelativeTo(sTemp).IgnoreResult();
      return true;
    }
  }

  ref_sPath.MakeRelativeTo(plFileSystem::GetSdkRootDirectory()).IgnoreResult();
  return false;
}

bool plQtEditorApp::MakePathDataDirectoryRelative(plString& ref_sPath) const
{
  plStringBuilder sTemp = ref_sPath;
  bool bRes = MakePathDataDirectoryRelative(sTemp);
  ref_sPath = sTemp;
  return bRes;
}
