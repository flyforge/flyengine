#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>

struct plPathPatternFilter;
class plAssetCurator;
class plApplicationFileSystemConfig;
class plPlatformProfile;
class plProgressRange;

//////////////////////////////////////////////////////////////////////////

struct PL_EDITORFRAMEWORK_DLL plProjectExport
{
  static plResult ExportProject(const char* szTargetDirectory, const plPlatformProfile* pPlatformProfile, const plApplicationFileSystemConfig& dataDirs);

private:
  struct DataDirectory
  {
    plString m_sTargetDirPath;
    plString m_sTargetDirRootName;
    plSet<plString> m_Files;
  };

  using DirectoryMapping = plMap<plString, DataDirectory>;

  static plResult ClearTargetFolder(const char* szAbsFolderPath);
  static plResult ScanFolder(plSet<plString>& out_Files, const char* szFolder, const plPathPatternFilter& filter, plAssetCurator* pCurator, plDynamicArray<plString>* pSceneFiles, const plPlatformProfile* pPlatformProfile);
  static plResult CopyFiles(const char* szSrcFolder, const char* szDstFolder, const plSet<plString>& files, plProgressRange* pProgressRange);
  static plResult GatherGeneratedAssetManagerFiles(plSet<plString>& out_Files);
  static plResult CreateExportFilterFile(const char* szExpectedFile, const char* szFallbackFile);
  static plResult ReadExportFilters(plPathPatternFilter& out_DataFilter, plPathPatternFilter& out_BinariesFilter, const plPlatformProfile* pPlatformProfile);
  static plResult CreateDataDirectoryDDL(const DirectoryMapping& mapping, const char* szTargetDirectory);
  static plResult GatherAssetLookupTableFiles(DirectoryMapping& mapping, const plApplicationFileSystemConfig& dirConfig, const plPlatformProfile* pPlatformProfile);
  static plResult ScanDataDirectories(DirectoryMapping& mapping, const plApplicationFileSystemConfig& dirConfig, const plPathPatternFilter& dataFilter, plDynamicArray<plString>* pSceneFiles, const plPlatformProfile* pPlatformProfile);
  static plResult CopyAllFiles(DirectoryMapping& mapping, const char* szTargetDirectory);
  static plResult GatherBinaries(DirectoryMapping& mapping, const plPathPatternFilter& filter);
  static plResult CreateLaunchConfig(const plDynamicArray<plString>& sceneFiles, const char* szTargetDirectory);
  static plResult GatherGeneratedAssetFiles(plSet<plString>& out_Files, const char* szProjectDirectory);
};
