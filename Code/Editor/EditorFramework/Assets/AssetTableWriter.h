#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Threading/TaskSystem.h>


struct plAssetCuratorEvent;
class plTask;
struct plAssetInfo;

/// \brief Asset table class. Persistent cache for an asset table.
///
/// The following assumptions need to be true for this cache to work:
/// 1. plAssetDocumentManager::AddEntriesToAssetTable does never change over time
/// 2. plAssetDocumentManager::GetAssetTableEntry never changes over the lifetime of an asset.
struct plAssetTable
{
  struct ManagerResource
  {
    plString m_sPath;
    plString m_sType;
  };

  plString m_sDataDir;
  plString m_sTargetFile;
  const plPlatformProfile* m_pProfile = nullptr;
  bool m_bDirty = true;
  bool m_bReset = true;
  plMap<plString, ManagerResource> m_GuidToManagerResource;
  plMap<plString, plString> m_GuidToPath;

  plResult WriteAssetTable();
  void Remove(const plSubAsset& subAsset);
  void Update(const plSubAsset& subAsset);
  void AddManagerResource(plStringView sGuid, plStringView sPath, plStringView sType);
};

/// \brief Keeps track of all asset tables and their state as well as reloading modified resources.
class PLASMA_EDITORFRAMEWORK_DLL plAssetTableWriter
{
public:
  plAssetTableWriter(const plApplicationFileSystemConfig& fileSystemConfig);
  ~plAssetTableWriter();

  /// \brief Needs to be called every frame. Handles update delays to allow compacting multiple changes.
  void MainThreadTick();

  /// \brief Marks an asset that needs to be reloaded in the engine process.
  /// The requests are batched and sent out via MainThreadTick.
  void NeedsReloadResource(const plUuid& assetGuid);

  /// \brief Writes the asset table for each data dir for the given asset profile.
  plResult WriteAssetTables(const plPlatformProfile* pAssetProfile, bool bForce);

private:
  void AssetCuratorEvents(const plAssetCuratorEvent& e);
  plAssetTable* GetAssetTable(plUInt32 uiDataDirIndex, const plPlatformProfile* pAssetProfile);
  plUInt32 FindDataDir(const plSubAsset& asset);

private:
  struct ReloadResource
  {
    plUInt32 m_uiDataDirIndex;
    plString m_sResource;
    plString m_sType;
  };

private:
  plApplicationFileSystemConfig m_FileSystemConfig;
  plDynamicArray<plString> m_DataDirRoots;

  mutable plCuratorMutex m_AssetTableMutex;
  bool m_bTablesDirty = true;
  bool m_bNeedToReloadResources = false;
  plTime m_NextTableFlush;
  plDynamicArray<ReloadResource> m_ReloadResources;
  plDeque<plMap<const plPlatformProfile*, plUniquePtr<plAssetTable>>> m_DataDirToAssetTables;
};
