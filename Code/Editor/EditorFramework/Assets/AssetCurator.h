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
#include <Foundation/Containers/DynamicArray.h>
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
#include <tuple>

class plUpdateTask;
class plTask;
class plAssetDocumentManager;
class plDirectoryWatcher;
class plProcessTask;
struct plFileStats;
class plAssetProcessorLog;
class plAssetWatcher;

#if 0 // Define to enable extensive curator profile scopes
#  define CURATOR_PROFILE(szName) PLASMA_PROFILE_SCOPE(szName)

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

struct PLASMA_EDITORFRAMEWORK_DLL plAssetInfo
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
    MissingDependency,
    MissingReference,
    Folder,
    COUNT,
  };

  plUInt8 m_LastStateUpdate = 0; ///< Changes every time m_TransformState is modified. Used to detect stale computations done outside the lock.
  plAssetExistanceState::Enum m_ExistanceState = plAssetExistanceState::FileAdded;
  TransformState m_TransformState = TransformState::Unknown;
  plUInt64 m_AssetHash = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.
  plUInt64 m_ThumbHash = 0; ///< Valid if m_TransformState != Unknown and asset not in Curator's m_TransformStateStale list.

  plDynamicArray<plLogEntry> m_LogEntries;

  const plAssetDocumentTypeDescriptor* m_pDocumentTypeDescriptor = nullptr;
  plString m_sAbsolutePath;
  plString m_sDataDirParentRelativePath;
  plString m_sDataDirRelativePath;

  plUniquePtr<plAssetDocumentInfo> m_Info;

  plSet<plString> m_MissingDependencies;
  plSet<plString> m_MissingReferences;

  plSet<plUuid> m_SubAssets; ///< Main asset uses the same GUID as this (see m_Info), but is NOT stored in m_SubAssets

private:
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plAssetInfo);
};

/// \brief Information about an asset or sub-asset.
struct PLASMA_EDITORFRAMEWORK_DLL plSubAsset
{
  plStringView GetName() const;
  void GetSubAssetIdentifier(plStringBuilder& out_sPath) const;

  plAssetExistanceState::Enum m_ExistanceState = plAssetExistanceState::FileAdded;
  plAssetInfo* m_pAssetInfo = nullptr;
  plTime m_LastAccess;
  bool m_bMainAsset = true;
  bool m_bIsDir = false;

  plSubAssetData m_Data;
};

/// \brief Information about a single file on disk. The file might be an asset or any other file (needed for dependencies).
struct PLASMA_EDITORFRAMEWORK_DLL plFileStatus
{
  enum class Status
  {
    Unknown,    ///< Since the file has been tagged as 'Unknown' it has not been encountered again on disk (yet)
    FileLocked, ///< The file is probably an asset, but we could not read it
    Valid       ///< The file exists on disk
  };

  plFileStatus()
  {
    m_uiHash = 0;
    m_Status = Status::Unknown;
  }

  plTimestamp m_Timestamp;
  plUInt64 m_uiHash;
  plUuid m_AssetGuid; ///< If the file is linked to an asset, the GUID is valid, otherwise not.
  Status m_Status;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plFileStatus);

struct plAssetCuratorEvent
{
  enum class Type
  {
    AssetAdded,
    AssetRemoved,
    AssetUpdated,
    AssetListReset,
    ActivePlatformChanged,
  };

  plUuid m_AssetGuid;
  const plSubAsset* m_pInfo;
  Type m_Type;
};

class PLASMA_EDITORFRAMEWORK_DLL plAssetCurator
{
  PLASMA_DECLARE_SINGLETON(plAssetCurator);

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
  /// \brief The main platform on which development happens. E.g. "PC".
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
  const plPlatformProfile* GetAssetProfile(plUInt32 index) const;

  /// \brief Always returns a valid config. E.g. even if plInvalidIndex is passed in, it will fall back to the default config (at index 0).
  plPlatformProfile* GetAssetProfile(plUInt32 index);

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
  void SetActiveAssetProfileByIndex(plUInt32 index, bool bForceReevaluation = false);

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

  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  plResult WriteAssetTables(const plPlatformProfile* pAssetProfile = nullptr);

  ///@}
  /// \name Asset Access
  ///@{
  typedef plLockedObject<plMutex, const plSubAsset> plLockedSubAsset;

  /// \brief Tries to find the asset information for an asset identified through a string.
  ///
  /// The string may be a stringyfied asset GUID or a relative or absolute path. The function will try all possibilities.
  /// If no asset can be found, an empty/invalid plAssetInfo is returned.
  /// If bExhaustiveSearch is set the function will go through all known assets and find the closest match.
  const plLockedSubAsset FindSubAsset(const char* szPathOrGuid, bool bExhaustiveSearch = false) const;

  /// \brief Same as GetAssteInfo, but wraps the return value into a plLockedSubAsset struct
  const plLockedSubAsset GetSubAsset(const plUuid& assetGuid) const;

  typedef plLockedObject<plMutex, const plHashTable<plUuid, plSubAsset>> plLockedSubAssetTable;

  /// \brief Returns the table of all known assets in a locked structure
  const plLockedSubAssetTable GetKnownSubAssets() const;

  /// \brief Computes the combined hash for the asset and its dependencies. Returns 0 if anything went wrong.
  plUInt64 GetAssetDependencyHash(plUuid assetGuid);

  /// \brief Computes the combined hash for the asset and its references. Returns 0 if anything went wrong.
  plUInt64 GetAssetReferenceHash(plUuid assetGuid);

  void GenerateTransitiveHull(const plStringView assetOrPath, plSet<plString>* pDependencies, plSet<plString>* pReferences);

  plAssetInfo::TransformState IsAssetUpToDate(const plUuid& assetGuid, const plPlatformProfile* pAssetProfile, const plAssetDocumentTypeDescriptor* pTypeDescriptor, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce = false);
  /// \brief Returns the number of assets in the system and how many are in what transform state
  void GetAssetTransformStats(plUInt32& out_uiNumAssets, plHybridArray<plUInt32, plAssetInfo::TransformState::COUNT>& out_count);

  /// \brief Iterates over all known data directories and returns the absolute path to the directory in which this asset is located
  plString FindDataDirectoryForAsset(const char* szAbsoluteAssetPath) const;

  /// \brief The curator gathers all folders.
  const plMap<plString, plFileStatus, plCompareString_NoCase>& GetAllAssetFolders() const { return m_AssetFolders; }
  plDynamicArray<plString>& GetRemovedFolders() { return m_RemovedFolders; }

  /// \brief Uses knowledge about all existing files on disk to find the best match for a file. Very slow.
  ///
  /// \param sFile
  ///   File name (may include a path) to search for. Will be modified both on success and failure to give a 'reasonable' result.
  plResult FindBestMatchForFile(plStringBuilder& sFile, plArrayPtr<plString> AllowedFileExtensions) const;

  /// \brief Finds all uses, either as references or dependencies to a given asset.
  ///
  /// Technically this finds all references and dependencies to this asset but in practice there are no uses of transform dependencies between assets right now so the result is a list of references and can be referred to as such.
  ///
  /// \param assetGuid
  ///   The asset to find use cases for.
  /// \param uses
  ///   List of assets that use 'assetGuid'.
  /// \param transitive
  ///   If set, will also find indirect uses of the asset.
  void FindAllUses(plUuid assetGuid, plSet<plUuid>& uses, bool transitive) const;

  ///@}
  /// \name Manual and Automatic Change Notification
  ///@{

  /// \brief Allows to tell the system of a new or changed file, that might be of interest to the Curator.
  void NotifyOfFileChange(const char* szAbsolutePath);

  /// \brief Allows to tell the system to re-evaluate an assets status.
  void NotifyOfAssetChange(const plUuid& assetGuid);
  void UpdateAssetLastAccessTime(const plUuid& assetGuid);

  /// \brief Checks file system for any changes. Call in case the file system watcher does not pick up certain changes.
  void CheckFileSystem();

  void NeedsReloadResources();

  ///@}

  void InvalidateAssetsWithTransformState(plAssetInfo::TransformState state);

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

  /// \brief Handles removing files and then forwards to HandleSingleFile overload.
  void HandleSingleFile(const plString& sAbsolutePath);
  /// \brief Handles adding and updating files. FileStat must be valid.
  void HandleSingleFile(const plString& sAbsolutePath, const plFileStats& FileStat);
  /// \brief Handles creating the minimal data structure for a directory and save it
  void HandleSingleDir(const plString& sAbsolutePath);
  /// \brief Handles creating the minimal data structure for a directory and save it
  void HandleSingleDir(const plString& sAbsolutePath, const plFileStats& FileStat);
  /// \brief Writes the asset lookup table for the given platform, or the currently active platform if nullptr is passed.
  plResult WriteAssetTable(const char* szDataDirectory, const plPlatformProfile* pAssetProfile = nullptr);
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

  plAssetInfo::TransformState HashAsset(plUInt64 uiSettingsHash, const plHybridArray<plString, 16>& assetTransformDependencies, const plHybridArray<plString, 16>& runtimeDependencies, plSet<plString>& missingDependencies, plSet<plString>& missingReferences, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce);
  bool AddAssetHash(plString& sPath, bool bIsReference, plUInt64& out_AssetHash, plUInt64& out_ThumbHash, bool bForce);

  plResult EnsureAssetInfoUpdated(const plUuid& assetGuid);
  plResult EnsureAssetInfoUpdated(const char* szAbsFilePath);
  void TrackDependencies(plAssetInfo* pAssetInfo);
  void UntrackDependencies(plAssetInfo* pAssetInfo);
  void UpdateTrackedFiles(const plUuid& assetGuid, const plSet<plString>& files, plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, plSet<std::tuple<plUuid, plUuid>>& unresolved, bool bAdd);
  void UpdateUnresolvedTrackedFiles(plMap<plString, plHybridArray<plUuid, 1>>& inverseTracker, plSet<std::tuple<plUuid, plUuid>>& unresolved);
  plResult ReadAssetDocumentInfo(const char* szAbsFilePath, plFileStatus& stat, plUniquePtr<plAssetInfo>& assetInfo);
  void UpdateSubAssets(plAssetInfo& assetInfo);
  /// \brief Computes the hash of the given file. Optionally passes the data stream through into another stream writer.
  static plUInt64 HashFile(plStreamReader& InputStream, plStreamWriter* pPassThroughStream);

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
  void RemoveStaleFileInfos();
  static void BuildFileExtensionSet(plSet<plString>& AllExtensions);
  void IterateDataDirectory(const char* szDataDir, plSet<plString>* pFoundFiles = nullptr);
  void LoadCaches();
  void SaveCaches();

  ///@}

private:
  friend class plUpdateTask;
  friend class plAssetProcessor;
  friend class plProcessTask;
  friend class plAssetWatcher;
  friend class plDirectoryUpdateTask;

  mutable plCuratorMutex m_CuratorMutex; // Global lock
  plTaskGroupID m_InitializeCuratorTaskID;
  bool m_bNeedToReloadResources = false;
  plTime m_NextReloadResources;
  plUInt32 m_uiActiveAssetProfile = 0;

  // Actual data stored in the curator
  plHashTable<plUuid, plAssetInfo*> m_KnownAssets;
  plHashTable<plUuid, plSubAsset> m_KnownSubAssets; //contains informations for both folders and "real" assets
  plMap<plString, plFileStatus, plCompareString_NoCase> m_ReferencedFiles;

  //Folders stuff
  plMap<plString, plFileStatus, plCompareString_NoCase> m_AssetFolders;
  plHashTable<plUuid, plAssetInfo*> m_KnownDirectories;
  plDynamicArray<plString> m_RemovedFolders;
  plAssetDocumentTypeDescriptor m_DirDescriptor;  //provides directories thumbnail info

  // Derived dependency lookup tables
  plMap<plString, plHybridArray<plUuid, 1>> m_InverseDependency;
  plMap<plString, plHybridArray<plUuid, 1>> m_InverseReferences;
  plSet<std::tuple<plUuid, plUuid>> m_UnresolvedDependencies; ///< If a dependency wasn't known yet when an asset info was loaded, it is put in here.
  plSet<std::tuple<plUuid, plUuid>> m_UnresolvedReferences;

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
  plSet<plString> m_ValidAssetExtensions;
  plUniquePtr<plAssetWatcher> m_pWatcher;

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
