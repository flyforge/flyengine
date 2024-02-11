#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Logging/LogEntry.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/LockedObject.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Timestamp.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/FileSystem/DataDirPath.h>
#include <ToolsFoundation/FileSystem/Declarations.h>

#include <tuple>

class plUpdateTask;
class plTask;
class plAssetDocumentManager;
class plDirectoryWatcher;
class plProcessTask;
struct plFileStats;
class plAssetProcessorLog;
class plFileSystemWatcher;
class plAssetTableWriter;
struct plFileChangedEvent;
class plFileSystemModel;


#if 0 // Define to enable extensive curator profile scopes
#  define CURATOR_PROFILE(szName) PL_PROFILE_SCOPE(szName)

#else
#  define CURATOR_PROFILE(Name)

#endif

/// \brief Custom mutex that allows to profile the time in the curator lock.
class plCuratorMutex : public plMutex
{
public:
  void Lock()
  {
    CURATOR_PROFILE("plCuratorMutex");
    plMutex::Lock();
  }

  void Unlock() { plMutex::Unlock(); }
};

struct PL_EDITORFRAMEWORK_DLL plAssetInfo
{
  plAssetInfo() = default;
  void Update(plUniquePtr<plAssetInfo>& rhs);

  plAssetDocumentManager* GetManager() { return static_cast<plAssetDocumentManager*>(m_pDocumentTypeDescriptor->m_pManager); }

  enum TransformState : plUInt8
  {
    Unknown = 0,
    UpToDate,
    NeedsImport,
    NeedsTransform,
    NeedsThumbnail,
    TransformError,
    MissingTransformDependency,
    MissingThumbnailDependency,
    CircularDependency,
    COUNT,
  };

  plUInt8 m_LastStateUpdate = 0; ///< Changes every time m_TransformState is modified. Used to detect stale computations done outside the lock.
  plAssetExistanceState::Enum m_ExistanceState = plAssetExistanceState::FileAdded;
  TransformState m_TransformState = TransformState::Unknown;
  plUInt64 m_AssetHash = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  plUInt64 m_ThumbHash = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.

  plDynamicArray<plLogEntry> m_LogEntries;

  const plAssetDocumentTypeDescriptor* m_pDocumentTypeDescriptor = nullptr;
  plDataDirPath m_Path;

  plUniquePtr<plAssetDocumentInfo> m_Info;

  plSet<plString> m_MissingTransformDeps;
  plSet<plString> m_MissingThumbnailDeps;
  plSet<plString> m_CircularDependencies;

  plSet<plUuid> m_SubAssets; ///< Main asset uses the same GUID as this (see m_Info), but is NOT stored in m_SubAssets

private:
  PL_DISALLOW_COPY_AND_ASSIGN(plAssetInfo);
};

/// \brief Information about an asset or sub-asset.
struct PL_EDITORFRAMEWORK_DLL plSubAsset
{
  plStringView GetName() const;
  void GetSubAssetIdentifier(plStringBuilder& out_sPath) const;

  plAssetExistanceState::Enum m_ExistanceState = plAssetExistanceState::FileAdded;
  plAssetInfo* m_pAssetInfo = nullptr;
  plTime m_LastAccess;
  bool m_bMainAsset = true;

  plSubAssetData m_Data;
};



struct plAssetCuratorEvent
{
  enum class Type
  {
    AssetAdded,
    AssetRemoved,
    AssetMoved,
    AssetUpdated,
    AssetListReset,
    ActivePlatformChanged,
  };

  plUuid m_AssetGuid;
  const plSubAsset* m_pInfo;
  Type m_Type;
};

class PL_EDITORFRAMEWORK_DLL plAssetCurator
{
  PL_DECLARE_SINGLETON(plAssetCurator);

public:
  plAssetCurator();
  ~plAssetCurator();

  /// \name Setup
  ///@{

  /// \brief Starts init task. Need to call WaitForInitialize to finish before loading docs.
  void StartInitialize(const plApplicationFileSystemConfig& cfg);
  /// \brief Waits for init task to finish.
  void WaitForInitialize();
  void Deinitialize();

  void MainThreadTick(bool bTopLevel);

  ///@}
  /// \name Asset Platform Configurations
  ///@{

public:
  /// \brief The main platform on which development happens. E.g. "Default".
  ///
  /// TODO: review this concept
  const plPlatformProfile* GetDevelopmentAssetProfile() const;

  /// \brief The currently active target platform for asset processing.
  const plPlatformProfile* GetActiveAssetProfile() const;

  /// \brief Returns the index of the currently active asset platform configuration
  plUInt32 GetActiveAssetProfileIndex() const;

  /// \brief Returns plInvalidIndex if no config with the given name exists. Name comparison is case insensitive.
  plUInt32 FindAssetProfileByName(const char* szPlatform);

  plUInt32 GetNumAssetProfiles() const;

  /// \brief Always returns a valid config. E.g. even if plInvalidIndex is passed in, it will fall back to the default config (at index 0).
  const plPlatformProfile* GetAssetProfile(plUInt32 uiIndex) const;

  /// \brief Always returns a valid config. E.g. even if plInvalidIndex is passed in, it will fall back to the default config (at index 0).
  plPlatformProfile* GetAssetProfile(plUInt32 uiIndex);

  /// \brief Adds a new profile. The name should be set afterwards to a unique name.
  plPlatformProfile* CreateAssetProfile();

  /// \brief Deletes the given asset profile, if possible.
  ///
  /// The function fails when the given profile is the main profile (at index 0),
  /// or it is the currently active profile.
  plResult DeleteAssetProfile(plPlatformProfile* pProfile);

  /// \brief Switches the currently active asset target platform.
  ///
  /// Broadcasts plAssetCuratorEvent::Type::ActivePlatformChanged on change.
  void SetActiveAssetProfileByIndex(plUInt32 uiIndex, bool bForceReevaluation = false);

  /// \brief Saves the current asset configurations. Returns failure if the output file could not be written to.
  plResult SaveAssetProfiles();

  void SaveRuntimeProfiles();

private:
  void ClearAssetProfiles();
  void SetupDefaultAssetProfiles();
  plResult LoadAssetProfiles();
  void ComputeAllDocumentManagerAssetProfileHashes();

  plHybridArray<plPlatformProfile*, 8> m_AssetProfiles;

  ///@}
  /// \name High Level Functions
  ///@{

public:
  plDateTime GetLastFullTransformDate() const;
  void StoreFullTransformDate();

  /// \brief Transforms all assets and writes the lookup tables. If the given platform is empty, the active platform is used.
  plStatus TransformAllAssets(plBitflags<plTransformFlags> transformFlags, const plPlatformProfile* pAssetProfile = nullptr);
  void ResaveAllAssets();
  plTransformStatus TransformAsset(const plUuid& assetGuid, plBitflags<plTransformFlags> transformFlags, const plPlatformProfile* pAssetProfile = nullptr);
  plTransformStatus CreateThumbnail(const plUuid& assetGuid);

  /// Some assets are not automatically updated by the asset dependency detection (mainly Collections) because of their transitive data dependencies.
  /// So we must update them when the user does something 'significant' like doing TransformAllAssets or a scene export.
  void TransformAssetsForSceneExport(const plPlatformProfile* pAssetProfile = nullptr);

  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  plResult WriteAssetTables(const plPlatformProfile* pAssetProfile = nullptr, bool bForce = false);

  ///@}
  /// \name Asset Access
  ///@{
  using plLockedSubAsset = plLockedObject<plMutex, const plSubAsset>;

  /// \brief Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be a stringyfied asset GUID or a relative or absolute path. The function will try all possibilities.
  /// If no asset can be found, an empty/invalid plAssetInfo is returned.
  /// If bExhaustiveSearch is set the function will go through all known assets and find the closest match.
  const plLockedSubAsset FindSubAsset(plStringView sPathOrGuid, bool bExhaustiveSearch = false) const;

  /// \brief Same as GetAssteInfo, but wraps the return value into a plLockedSubAsset struct
  const plLockedSubAsset GetSubAsset(const plUuid& assetGuid) const;

  using plLockedSubAssetTable = plLockedObject<plMutex, const plHashTable<plUuid, plSubAsset>>;

  /// \brief Returns the table of all known assets in a locked structure
  const plLockedSubAssetTable GetKnownSubAssets() const;

  using plLockedAssetTable = plLockedObject<plMutex, const plHashTable<plUuid, plAssetInfo*>>;

  /// \brief Returns the table of all known assets in a locked structure
  const plLockedAssetTable GetKnownAssets() const;

  /// \brief Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  plUInt64 GetAssetDependencyHash(plUuid assetGuid);

  /// \brief Computes the combined hash for the asset and its references. Returns 0 if anything went wrong.
  plUInt64 GetAssetReferenceHash(plUuid assetGuid);

  plAssetInfo::TransformState IsAssetUpToDate(const plUuid& assetGuid, const plPlatformProfile* pAssetProfile, const plAssetDocumentTypeDescriptor* pTypeDescriptor, plUInt64& out_uiAssetHash, plUInt64& out_uiThumbHash, bool bForce = false);
  /// \brief Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(plUInt32& out_uiNumAssets, plHybridArray<plUInt32, plAssetInfo::TransformState::COUNT>& out_count);

  /// \brief Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  plString FindDataDirectoryForAsset(plStringView sAbsoluteAssetPath) const;

  /// \brief Uses knowledge about all existing files on disk to find the best match for a file. Very slow.
  ///
  /// \param sFile
  ///   File name (may include a path) to search for. Will be modified both on success and failure to give a 'reasonable' result.
  plResult FindBestMatchForFile(plStringBuilder& ref_sFile, plArrayPtr<plString> allowedFileExtensions) const;

  /// \brief Finds all uses, either as references or dependencies to a given asset.
  ///
  /// Technically this finds all references and dependencies to this asset but in practice there are no uses of transform dependencies between assets right now so the result is a list of references and can be referred to as such.
  ///
  /// \param assetGuid
  ///   The asset to find use cases for.
  /// \param uses
  ///   List of assets that use 'assetGuid'. Any previous content of the set is not removed.
  /// \param transitive
  ///   If set, will also find indirect uses of the asset.
  void FindAllUses(plUuid assetGuid, plSet<plUuid>& ref_uses, bool bTransitive) const;

  /// \brief Returns all assets that use a file for transform. Use this to e.g. figure which assets still reference a .tga file in the project.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \param ref_uses List of assets that use 'sAbsolutePath'. Any previous content of the set is not removed.
  void FindAllUses(plStringView sAbsolutePath, plSet<plUuid>& ref_uses) const;

  /// \brief Returns whether a file is referenced, i.e. used for transforming an asset. Use this to e.g. figure out whether a .tga file is still in use by any asset.
  /// \param sAbsolutePath Absolute path to any file inside a data directory.
  /// \return True, if at least one asset references the given file.
  bool IsReferenced(plStringView sAbsolutePath) const;


  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

  /// \brief Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfFileChange(plStringView sAbsolutePath);
  /// \brief Allows to tell the system to re-evaluate an assets status.
  void NotifyOfAssetChange(const plUuid& assetGuid);
  void UpdateAssetLastAccessTime(const plUuid& assetGuid);

  /// \brief Checks file system for any changes. Call in case the file system watcher does not pick up certain changes.
  void CheckFileSystem();

  void NeedsReloadResources(const plUuid& assetGuid);

  void InvalidateAssetsWithTransformState(plAssetInfo::TransformState state);


  ///@}

  /// \name Utilities
  ///@{

  /// \brief Generates one transitive hull for all the dependencies that are enabled. The set will contain dependencies that are reachable via any combination of enabled reference types.
  void GenerateTransitiveHull(const plStringView sAssetOrPath, plSet<plString>& inout_deps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false, bool bIncludePackageDeps = false) const;

  /// \brief Generates one inverse transitive hull for all the types dependencies that are enabled. The set will contain inverse dependencies that can reach the given asset (pAssetInfo) via any combination of the enabled reference types. As only assets can have dependencies, the inverse hull is always just asset GUIDs.
  void GenerateInverseTransitiveHull(const plAssetInfo* pAssetInfo, plSet<plUuid>& inout_inverseDeps, bool bIncludeTransformDeps = false, bool bIncludeThumbnailDeps = false) const;

  /// \brief Generates a DGML graph of all transform and thumbnail dependencies.
  void WriteDependencyDGML(const plUuid& guid, plStringView sOutputFile) const;

  ///@}

public:
  plEvent<const plAssetCuratorEvent&> m_Events;

private:
  /// \name Processing
  ///@{

  plTransformStatus ProcessAsset(plAssetInfo* pAssetInfo, const plPlatformProfile* pAssetProfile, plBitflags<plTransformFlags> transformFlags);
  plStatus ResaveAsset(plAssetInfo* pAssetInfo);
  /// \brief Returns the asset info for the asset with the given GUID or nullptr if no such asset exists.
  plAssetInfo* GetAssetInfo(const plUuid& assetGuid);
  const plAssetInfo* GetAssetInfo(const plUuid& assetGuid) const;

  plSubAsset* GetSubAssetInternal(const plUuid& assetGuid);

  /// \brief Returns the asset info for the asset with the given (stringyfied) GUID or nullptr if no such asset exists.
  plAssetInfo* GetAssetInfo(const plString& sAssetGuid);

  void OnFileChangedEvent(const plFileChangedEvent& e);

  /// \brief Some assets are vital for the engine to run. Each data directory can contain a [DataDirName].plCollectionAsset
  ///   that has all its references transformed before any other documents are loaded.
  void ProcessAllCoreAssets();

  ///@}
  /// \name Update Task
  ///@{

  void RestartUpdateTask();
  void ShutdownUpdateTask();

  bool GetNextAssetToUpdate(plUuid& out_guid, plStringBuilder& out_sAbsPath);
  void OnUpdateTaskFinished(const plSharedPtr<plTask>& pTask);
  void RunNextUpdateTask();

  ///@}
  /// \name Asset Hashing and Status Updates (AssetUpdates.cpp)
  ///@{

  plAssetInfo::TransformState HashAsset(
    plUInt64 uiSettingsHash, const plHybridArray<plString, 16>& assetTransformDeps, const plHybridArray<plString, 16>& assetThumbnailDeps, plSet<plString>& missingTransformDeps, plSet<plString>& missingThumbnailDeps, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce);
  bool AddAssetHash(plString& sPath, bool bIsReference, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce);

  plResult EnsureAssetInfoUpdated(const plDataDirPath& absFilePath, const plFileStatus& stat, bool bForce = false);
  void TrackDependencies(plAssetInfo* pAssetInfo);
  void UntrackDependencies(plAssetInfo* pAssetInfo);
  plResult CheckForCircularDependencies(plAssetInfo* pAssetInfo);
  void UpdateTrackedFiles(const plUuid& assetGuid, const plSet<plString>& files, plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, plSet<std::tuple<plUuid, plUuid>>& unresolved, bool bAdd);
  void UpdateUnresolvedTrackedFiles(plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, plSet<std::tuple<plUuid, plUuid>>& unresolved);
  plResult ReadAssetDocumentInfo(const plDataDirPath& absFilePath, const plFileStatus& stat, plUniquePtr<plAssetInfo>& assetInfo);
  void UpdateSubAssets(plAssetInfo& assetInfo);

  void RemoveAssetTransformState(const plUuid& assetGuid);
  void InvalidateAssetTransformState(const plUuid& assetGuid);

  plAssetInfo::TransformState UpdateAssetTransformState(plUuid assetGuid, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce);
  void UpdateAssetTransformState(const plUuid& assetGuid, plAssetInfo::TransformState state);
  void UpdateAssetTransformLog(const plUuid& assetGuid, plDynamicArray<plLogEntry>& logEntries);
  void SetAssetExistanceState(plAssetInfo& assetInfo, plAssetExistanceState::Enum state);

  ///@}
  /// \name Check File System Helper
  ///@{
  void SetAllAssetStatusUnknown();
  void LoadCaches(plMap<plDataDirPath, plFileStatus, plCompareDataDirPath>& out_referencedFiles, plMap<plDataDirPath, plFileStatus::Status, plCompareDataDirPath>& out_referencedFolders);
  void SaveCaches(const plMap<plDataDirPath, plFileStatus, plCompareDataDirPath>& referencedFiles, const plMap<plDataDirPath, plFileStatus::Status, plCompareDataDirPath>& referencedFolders);
  static void BuildFileExtensionSet(plSet<plString>& AllExtensions);

  ///@}
  /// \name Utilities
  ///@{

public:
  /// \brief Deletes all files in all asset caches, except for the asset outputs that exceed the threshold.
  ///
  /// -> OutputReliability::Perfect -> deletes everything
  /// -> OutputReliability::Good -> keeps the 'Perfect' files
  /// -> OutputReliability::Unknown -> keeps the 'Good' and 'Perfect' files
  void ClearAssetCaches(plAssetDocumentManager::OutputReliability threshold);

  ///@}

private:
  friend class plUpdateTask;
  friend class plAssetProcessor;
  friend class plProcessTask;

  mutable plCuratorMutex m_CuratorMutex; // Global lock
  plTaskGroupID m_InitializeCuratorTaskID;

  plUInt32 m_uiActiveAssetProfile = 0;

  // Actual data stored in the curator
  plHashTable<plUuid, plAssetInfo*> m_KnownAssets;
  plHashTable<plUuid, plSubAsset> m_KnownSubAssets;

  // Derived dependency lookup tables
  plMap<plString, plHybridArray<plUuid, 1>> m_InverseTransformDeps; // [Absolute path -> asset Guid]
  plMap<plString, plHybridArray<plUuid, 1>> m_InverseThumbnailDeps; // [Absolute path -> asset Guid]
  plSet<std::tuple<plUuid, plUuid>> m_UnresolvedTransformDeps;      ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  plSet<std::tuple<plUuid, plUuid>> m_UnresolvedThumbnailDeps;

  // State caches
  plHashSet<plUuid> m_TransformState[plAssetInfo::TransformState::COUNT];
  plHashSet<plUuid> m_SubAssetChanged; ///< Flushed in main thread tick
  plHashSet<plUuid> m_TransformStateStale;
  plHashSet<plUuid> m_Updating;

  // Serialized cache
  mutable plCuratorMutex m_CachedAssetsMutex; ///< Only locks m_CachedAssets
  plMap<plString, plUniquePtr<plAssetDocumentInfo>> m_CachedAssets;
  plMap<plString, plFileStatus> m_CachedFiles;

  // Immutable data after StartInitialize
  plApplicationFileSystemConfig m_FileSystemConfig;
  plUniquePtr<plAssetTableWriter> m_pAssetTableWriter;
  plSet<plString> m_ValidAssetExtensions;

  // Update task
  bool m_bRunUpdateTask = false;
  plSharedPtr<plUpdateTask> m_pUpdateTask;
  plTaskGroupID m_UpdateTaskGroup;
};

class plUpdateTask final : public plTask
{
public:
  plUpdateTask(plOnTaskFinishedCallback onTaskFinished);
  ~plUpdateTask();

private:
  plStringBuilder m_sAssetPath;

  virtual void Execute() override;
};
