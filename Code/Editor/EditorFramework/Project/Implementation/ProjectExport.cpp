#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

plResult plProjectExport::ClearTargetFolder(const char* szAbsFolderPath)
{
  if (plOSFile::DeleteFolder(szAbsFolderPath).Failed())
  {
    plLog::Error("Target folder could not be removed:\n'{}'", szAbsFolderPath);
    return PL_FAILURE;
  }

  if (plOSFile::CreateDirectoryStructure(szAbsFolderPath).Failed())
  {
    plLog::Error("Target folder could not be created:\n'{}'", szAbsFolderPath);
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plProjectExport::ScanFolder(plSet<plString>& out_Files, const char* szFolder, const plPathPatternFilter& filter, plAssetCurator* pCurator, plDynamicArray<plString>* pSceneFiles, const plPlatformProfile* pPlatformProfile)
{
  plStringBuilder sRootFolder = szFolder;
  sRootFolder.Trim("/\\");

  const plUInt32 uiRootFolderLength = sRootFolder.GetElementCount();

  plStringBuilder sAbsFilePath, sRelFilePath;

  plFileSystemIterator it;
  for (it.StartSearch(sRootFolder, plFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
  {
    if (plProgress::GetGlobalProgressbar()->WasCanceled())
    {
      plLog::Warning("Folder scanning canceled by user");
      return PL_FAILURE;
    }

    it.GetStats().GetFullPath(sAbsFilePath);

    sRelFilePath = sAbsFilePath;
    sRelFilePath.Shrink(uiRootFolderLength, 0); // keep the slash at the front -> useful for the pattern filter

    if (!filter.PassesFilters(sRelFilePath))
    {
      if (it.GetStats().m_bIsDirectory)
        it.SkipFolder();
      else
        it.Next();

      continue;
    }

    if (it.GetStats().m_bIsDirectory)
    {
      it.Next();
      continue;
    }

    if (pCurator)
    {
      auto asset = pCurator->FindSubAsset(sAbsFilePath);

      if (asset.isValid() && asset->m_bMainAsset)
      {
        // redirect to asset output
        plAssetDocumentManager* pAssetMan = plStaticCast<plAssetDocumentManager*>(asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_pManager);

        sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_Path, nullptr, pPlatformProfile);

        sRelFilePath.Prepend("AssetCache/");
        out_Files.Insert(sRelFilePath);

        if (pSceneFiles && asset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sDocumentTypeName == "Scene")
        {
          pSceneFiles->PushBack(sRelFilePath);
        }

        for (const plString& outputTag : asset->m_pAssetInfo->m_Info->m_Outputs)
        {
          sRelFilePath = pAssetMan->GetRelativeOutputFileName(asset->m_pAssetInfo->m_pDocumentTypeDescriptor, sRootFolder, asset->m_pAssetInfo->m_Path, outputTag, pPlatformProfile);

          sRelFilePath.Prepend("AssetCache/");
          out_Files.Insert(sRelFilePath);
        }

        it.Next();
        continue;
      }
    }

    out_Files.Insert(sRelFilePath);
    it.Next();
  }

  return PL_SUCCESS;
}

plResult plProjectExport::CopyFiles(const char* szSrcFolder, const char* szDstFolder, const plSet<plString>& files, plProgressRange* pProgressRange)
{
  plLog::Info("Source folder: {}", szSrcFolder);
  plLog::Info("Destination folder: {}", szDstFolder);

  plStringBuilder sSrc, sDst;

  for (auto itFile = files.GetIterator(); itFile.IsValid(); ++itFile)
  {
    if (plProgress::GetGlobalProgressbar()->WasCanceled())
    {
      plLog::Info("File copy operation canceled by user.");
      return PL_FAILURE;
    }

    if (pProgressRange)
    {
      pProgressRange->BeginNextStep(itFile.Key());
    }

    sSrc.Set(szSrcFolder, "/", itFile.Key());
    sDst.Set(szDstFolder, "/", itFile.Key());

    if (plOSFile::CopyFile(sSrc, sDst).Succeeded())
    {
      plLog::Info(" Copied: {}", itFile.Key());
    }
    else
    {
      plLog::Error(" Copy failed: {}", itFile.Key());
    }
  }

  plLog::Success("Finished copying files to destination '{}'", szDstFolder);
  return PL_SUCCESS;
}

plResult plProjectExport::GatherGeneratedAssetManagerFiles(plSet<plString>& out_Files)
{
  plHybridArray<plString, 4> addFiles;

  for (auto pMan : plDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = plDynamicCast<plAssetDocumentManager*>(pMan))
    {
      pAssMan->GetAdditionalOutputs(addFiles).AssertSuccess();

      for (const auto& file : addFiles)
      {
        out_Files.Insert(file);
      }

      addFiles.Clear();
    }
  }

  return PL_SUCCESS;
}

plResult plProjectExport::CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile)
{
  if (plFileSystem::ExistsFile(szExpectedFile))
    return PL_SUCCESS;

  plStringBuilder src;
  src.Set("#include <", szFallbackFile, ">\n\n\n[EXCLUDE]\n\n// TODO: add exclude patterns\n\n\n[INCLUDE]\n\n//TODO: add include patterns\n\n\n");

  plFileWriter file;
  if (file.Open(szExpectedFile).Failed())
  {
    plLog::Error("Failed to open '{}' for writing.", szExpectedFile);
    return PL_FAILURE;
  }

  file.WriteBytes(src.GetData(), src.GetElementCount()).AssertSuccess();
  return PL_SUCCESS;
}

plResult plProjectExport::ReadExportFilters(plPathPatternFilter& out_DataFilter, plPathPatternFilter& out_BinariesFilter, const plPlatformProfile* pPlatformProfile)
{
  plStringBuilder sDefine;
  sDefine.SetFormat("PLATFORM_PROFILE_{} 1", pPlatformProfile->GetTargetPlatform());
  sDefine.ToUpper();

  plHybridArray<plString, 1> ppDefines;
  ppDefines.PushBack(sDefine);

  if (plProjectExport::CreateExportFilterFile(":project/ProjectData.plExportFilter", "CommonData.plExportFilter").Failed())
  {
    plLog::Error("The file 'ProjectData.plExportFilter' could not be created.");
    return PL_FAILURE;
  }

  if (plProjectExport::CreateExportFilterFile(":project/ProjectBinaries.plExportFilter", "CommonBinaries.plExportFilter").Failed())
  {
    plLog::Error("The file 'ProjectBinaries.plExportFilter' could not be created.");
    return PL_FAILURE;
  }

  if (out_DataFilter.ReadConfigFile("ProjectData.plExportFilter", ppDefines).Failed())
  {
    plLog::Error("The file 'ProjectData.plExportFilter' could not be read.");
    return PL_FAILURE;
  }

  if (out_BinariesFilter.ReadConfigFile("ProjectBinaries.plExportFilter", ppDefines).Failed())
  {
    plLog::Error("The file 'ProjectBinaries.plExportFilter' could not be read.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plProjectExport::CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory)
{
  plApplicationFileSystemConfig cfg;

  plStringBuilder sPath;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    const auto& info = itDir.Value();

    if (info.m_sTargetDirRootName == "-")
      continue;

    sPath.Set(">sdk/", info.m_sTargetDirPath);

    auto& ddc = cfg.m_DataDirs.ExpandAndGetRef();
    ddc.m_sDataDirSpecialPath = sPath;
    ddc.m_sRootName = info.m_sTargetDirRootName;
  }

  sPath.Set(szTargetDirectory, "/Data/project/RuntimeConfigs/DataDirectories.ddl");

  if (cfg.Save(sPath).Failed())
  {
    plLog::Error("Failed to write DataDirectories.ddl file.");
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plProjectExport::GatherAssetLookupTableFiles(DirectoryMapping& mapping, const plApplicationFileSystemConfig& dirConfig, const plPlatformProfile* pPlatformProfile)
{
  plStringBuilder sDataDirPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    if (plFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      plLog::Error("Failed to resolve data directory path '{}'", dataDir.m_sDataDirSpecialPath);
      return PL_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    plStringBuilder sAidltPath("AssetCache/", pPlatformProfile->GetConfigName(), ".plAidlt");

    mapping[sDataDirPath].m_Files.Insert(sAidltPath);
  }

  return PL_SUCCESS;
}

plResult plProjectExport::ScanDataDirectories(DirectoryMapping& mapping, const plApplicationFileSystemConfig& dirConfig, const plPathPatternFilter& dataFilter, plDynamicArray<plString>* pSceneFiles, const plPlatformProfile* pPlatformProfile)
{
  plProgressRange progress("Scanning data directories", dirConfig.m_DataDirs.GetCount(), true);

  plUInt32 uiDataDirNumber = 1;

  plStringBuilder sDataDirPath, sDstPath;

  for (const auto& dataDir : dirConfig.m_DataDirs)
  {
    progress.BeginNextStep(dataDir.m_sDataDirSpecialPath);

    if (plFileSystem::ResolveSpecialDirectory(dataDir.m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      plLog::Error("Failed to get special directory '{0}'", dataDir.m_sDataDirSpecialPath);
      return PL_FAILURE;
    }

    sDataDirPath.Trim("/\\");

    plProjectExport::DataDirectory& ddInfo = mapping[sDataDirPath];

    if (!dataDir.m_sRootName.IsEmpty())
    {
      sDstPath.Set("Data/", dataDir.m_sRootName);

      ddInfo.m_sTargetDirRootName = dataDir.m_sRootName;
      ddInfo.m_sTargetDirPath = sDstPath;
    }
    else
    {
      sDstPath.SetFormat("Data/Extra{}", uiDataDirNumber);
      ++uiDataDirNumber;

      ddInfo.m_sTargetDirPath = sDstPath;
    }

    PL_SUCCEED_OR_RETURN(plProjectExport::ScanFolder(ddInfo.m_Files, sDataDirPath, dataFilter, plAssetCurator::GetSingleton(), pSceneFiles, pPlatformProfile));
  }

  return PL_SUCCESS;
}

plResult plProjectExport::CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory)
{
  plUInt32 uiTotalFiles = 0;
  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
    uiTotalFiles += itDir.Value().m_Files.GetCount();

  plProgressRange range("Copying files", uiTotalFiles, true);

  plLog::Info("Copying files to target directory '{}'", szTargetDirectory);

  plStringBuilder sTargetFolder;

  for (auto itDir = mapping.GetIterator(); itDir.IsValid(); ++itDir)
  {
    sTargetFolder.Set(szTargetDirectory, "/", itDir.Value().m_sTargetDirPath);

    if (plProjectExport::CopyFiles(itDir.Key(), sTargetFolder, itDir.Value().m_Files, &range).Failed())
      return PL_FAILURE;
  }

  plLog::Success("Finished copying all files.");
  return PL_SUCCESS;
}

plResult plProjectExport::GatherBinaries(DirectoryMapping& mapping, const plPathPatternFilter& filter)
{
  plStringBuilder sAppDir;
  sAppDir = plOSFile::GetApplicationDirectory();
  sAppDir.MakeCleanPath();
  sAppDir.Trim("/\\");

  plProjectExport::DataDirectory& ddInfo = mapping[sAppDir];
  ddInfo.m_sTargetDirPath = "Bin";
  ddInfo.m_sTargetDirRootName = "-"; // don't add to data dir config

  if (plProjectExport::ScanFolder(ddInfo.m_Files, sAppDir, filter, nullptr, nullptr, nullptr).Failed())
    return PL_FAILURE;

  return PL_SUCCESS;
}

plResult plProjectExport::CreateLaunchConfig(const plDynamicArray<plString>& sceneFiles, const char* szTargetDirectory)
{
  for (const auto& sf : sceneFiles)
  {
    plStringBuilder cmd;
    cmd.SetFormat("start Bin/Player.exe -project \"Data/project\" -scene \"{}\"", sf);

    plStringBuilder bat;
    bat.SetFormat("{}/Launch {}.bat", szTargetDirectory, plPathUtils::GetFileName(sf));

    plOSFile file;
    if (file.Open(bat, plFileOpenMode::Write).Failed())
    {
      plLog::Error("Couldn't create '{}'", bat);
      return PL_FAILURE;
    }

    file.Write(cmd.GetData(), cmd.GetElementCount()).AssertSuccess();
  }

  return PL_SUCCESS;
}

plResult plProjectExport::GatherGeneratedAssetFiles(plSet<plString>& out_Files, const char* szProjectDirectory)
{
  plStringBuilder sRoot(szProjectDirectory, "/AssetCache/Generated");

  plPathPatternFilter filter;
  plSet<plString> files;
  PL_SUCCEED_OR_RETURN(ScanFolder(files, sRoot, filter, nullptr, nullptr, nullptr));

  plStringBuilder sFilePath;

  for (const auto& file : files)
  {
    sFilePath.Set("/AssetCache/Generated", file);
    out_Files.Insert(sFilePath);
  }

  return PL_SUCCESS;
}

plResult plProjectExport::ExportProject(const char* szTargetDirectory, const plPlatformProfile* pPlatformProfile, const plApplicationFileSystemConfig& dataDirs)
{
  plProgressRange mainProgress("Export Project", 7, true);
  mainProgress.SetStepWeighting(0, 0.05f); // Preparing output folder
  mainProgress.SetStepWeighting(1, 0.05f); // Generating special files
  mainProgress.SetStepWeighting(2, 0.10f); // Scanning data directories
  mainProgress.SetStepWeighting(3, 0.05f); // Gathering binaries
  mainProgress.SetStepWeighting(4, 1.0f);  // Copying files
  mainProgress.SetStepWeighting(5, 0.01f); // Writing data directory config
  mainProgress.SetStepWeighting(6, 0.01f); // Finish up

  plStringBuilder sProjectRootDir;
  plHybridArray<plString, 16> sceneFiles;
  plProjectExport::DirectoryMapping fileList;

  plPathPatternFilter dataFilter;
  plPathPatternFilter binariesFilter;

  // 0
  {
    mainProgress.BeginNextStep("Preparing output folder");
    PL_SUCCEED_OR_RETURN(plProjectExport::ClearTargetFolder(szTargetDirectory));
  }

  // 0
  {
    plFileSystem::ResolveSpecialDirectory(">project", sProjectRootDir).AssertSuccess();
    sProjectRootDir.Trim("/\\");

    PL_SUCCEED_OR_RETURN(plProjectExport::GatherAssetLookupTableFiles(fileList, dataDirs, pPlatformProfile));
    PL_SUCCEED_OR_RETURN(plProjectExport::ReadExportFilters(dataFilter, binariesFilter, pPlatformProfile));
    PL_SUCCEED_OR_RETURN(plProjectExport::GatherGeneratedAssetFiles(fileList[sProjectRootDir].m_Files, sProjectRootDir));
  }

  // 1
  {
    mainProgress.BeginNextStep("Generating special files");
    PL_SUCCEED_OR_RETURN(plProjectExport::GatherGeneratedAssetManagerFiles(fileList[sProjectRootDir].m_Files));
  }

  // 2
  {
    mainProgress.BeginNextStep("Scanning data directories");
    PL_SUCCEED_OR_RETURN(plProjectExport::ScanDataDirectories(fileList, dataDirs, dataFilter, &sceneFiles, pPlatformProfile));
  }

  // 3
  {
    // by default all DLLs are excluded by CommonBinaries.plExportFilter
    // we want to override this for all the runtime DLLs and indirect DLL dependencies
    // so we add those to the 'include filter'

    for (auto it : plQtEditorApp::GetSingleton()->GetPluginBundles().m_Plugins)
    {
      if (!it.Value().m_bSelected)
        continue;

      for (const auto& dep : it.Value().m_PackageDependencies)
      {
        binariesFilter.AddFilter(dep, true);
      }

      for (const auto& dep : it.Value().m_RuntimePlugins)
      {
        plStringBuilder tmp = dep;

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
        tmp.Append(".dll");
#elif PL_ENABLED(PL_PLATFORM_LINUX)
        tmp.Append(".so");
#else
#  error "Platform not implemented"
#endif

        binariesFilter.AddFilter(tmp, true);
      }
    }

    mainProgress.BeginNextStep("Gathering binaries");
    PL_SUCCEED_OR_RETURN(plProjectExport::GatherBinaries(fileList, binariesFilter));
  }

  // 4
  {
    mainProgress.BeginNextStep("Copying files");
    PL_SUCCEED_OR_RETURN(plProjectExport::CopyAllFiles(fileList, szTargetDirectory));
  }

  // 5
  {
    mainProgress.BeginNextStep("Writing data directory config");
    PL_SUCCEED_OR_RETURN(plProjectExport::CreateDataDirectoryDDL(fileList, szTargetDirectory));
  }

  // 6
  {
    mainProgress.BeginNextStep("Finishing up");
    PL_SUCCEED_OR_RETURN(plProjectExport::CreateLaunchConfig(sceneFiles, szTargetDirectory));
  }

  return PL_SUCCESS;
}
