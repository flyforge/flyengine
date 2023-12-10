#pragma once

#include <ToolsFoundation/ToolsFoundationDLL.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DIRECTORY_WATCHER) && PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)

#  include <Foundation/Application/Config/FileSystemConfig.h>
#  include <Foundation/Configuration/Singleton.h>
#  include <Foundation/Threading/LockedObject.h>
#  include <Foundation/Types/UniquePtr.h>
#  include <ToolsFoundation/FileSystem/DataDirPath.h>
#  include <ToolsFoundation/FileSystem/Declarations.h>

class plFileSystemWatcher;
struct plFileSystemWatcherEvent;
struct plFileStats;

/// \brief Event fired by plFileSystemModel::m_FolderChangedEvents
struct PLASMA_TOOLSFOUNDATION_DLL plFolderChangedEvent
{
  enum class Type
  {
    None,
    FolderAdded,
    FolderRemoved,
    ModelReset, ///< Model was initialized or deinitialized.
  };

  plFolderChangedEvent() = default;
  plFolderChangedEvent(const plDataDirPath& file, Type type);

  plDataDirPath m_Path;
  Type m_Type = Type::None;
};

/// \brief Event fired by plFileSystemModel::m_FileChangedEvents
struct PLASMA_TOOLSFOUNDATION_DLL plFileChangedEvent
{
  enum class Type
  {
    None,
    FileAdded,
    FileChanged,
    DocumentLinked,
    DocumentUnlinked,
    FileRemoved,
    ModelReset ///< Model was initialized or deinitialized.
  };

  plFileChangedEvent() = default;
  plFileChangedEvent(const plDataDirPath& file, plFileStatus status, Type type);

  plDataDirPath m_Path;
  plFileStatus m_Status;
  Type m_Type = Type::None;
};

/// \brief A subsystem for tracking all files in a plApplicationFileSystemConfig.
///
/// Once Initialize is called with the plApplicationFileSystemConfig to track, the current state should be updated by calling CheckFileSystem() on a worker thread. This will trigger m_FolderChangedEvents and m_FileChangedEvents for all files / folders found in the data directories present in the config. Any future changes will be picked up by the plFileSystemWatcher created in Initialize.
/// For the system to work, the MainThreadTick function needs to be called at regular (e.g. frame) intervals.
/// The model also caches file hashes as well as allows files to be linked to document GUIDs for fast lookups.
class PLASMA_TOOLSFOUNDATION_DLL plFileSystemModel
{
  PLASMA_DECLARE_SINGLETON(plFileSystemModel);

public:
  using FilesMap = plMap<plDataDirPath, plFileStatus, plCompareDataDirPath>;
  using FoldersMap = plMap<plDataDirPath, plFileStatus::Status, plCompareDataDirPath>;

  using LockedFiles = plLockedObject<plMutex, const FilesMap>;
  using LockedFolders = plLockedObject<plMutex, const FoldersMap>;

public:
  /// \brief Return true if the two paths point to the same file on disk. On different platforms the same strings can produce different results. This function assumes both paths are absolute and cleaned via plStringBuilder::MakeCleanPath.
  static bool IsSameFile(const plStringView sAbsolutePathA, const plStringView sAbsolutePathB);

  /// \brief Computes the hash of the given file. Optionally passes the data stream through into another stream writer.
  static plUInt64 HashFile(plStreamReader& ref_inputStream, plStreamWriter* pPassThroughStream);

public:
  /// \name Setup
  ///@{

  plFileSystemModel();
  ~plFileSystemModel();

  /// \brief Initializes the model for the given file system config.
  /// \param fileSystemConfig All data directories in this config will be tracked by the model.
  /// \param referencedFiles Restores the previous state of the file model. E.g. cached on disk. If the plFileStatus::Status is plFileStatus::Status::Unknown m_FileChangedEvents is guaranteed to be fired once the file is checked again, e.g. via CheckFileSystem or NotifyOfChange.
  /// \param referencedFolders Restores the previous state of the folder model. E.g. cached on disk.
  void Initialize(const plApplicationFileSystemConfig& fileSystemConfig, FilesMap&& referencedFiles, FoldersMap&& referencedFolders);

  /// \brief Deinitialize the model.
  /// \param out_pReferencedFiles If set, filled with the current state of the file model so it can be cached, e.g. by storing it on disk.
  /// \param out_pReferencedFolders If set, filled with the current state of the folder model so it can be cached, e.g. by storing it on disk.
  void Deinitialize(FilesMap* out_pReferencedFiles = nullptr, FoldersMap* out_pReferencedFolders = nullptr);

  /// \brief Needs to be called every frame to restart background tasks.
  void MainThreadTick();

  const plApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  plArrayPtr<const plString> GetDataDirectoryRoots() const { return m_DataDirRoots.GetArrayPtr(); }

  ///@}
  /// \name File / Folder Access
  ///@{

  /// \brief Returns all files in the model.
  /// \return Returns the files and also a lock to the model.
  const LockedFiles GetFiles() const;

  /// \brief Returns all folders in the model.
  /// \return Returns the folders and also a lock to the model.
  const LockedFolders GetFolders() const;

  /// \brief Searches for a file in the model.
  /// \param sPath Absolute or relative path to a file to be searched for.
  /// \param stat Contains the current state of the file in the model if found.
  /// \return Returns PLASMA_SUCCESS if the file was found.
  plResult FindFile(plStringView sPath, plFileStatus& out_stat) const;

  /// \brief Searches for the first file in the model that satisfies the given visitor function.
  /// \param visitor Called for every file in the model. If this functions returns true, the search is canceled and the function returns PLASMA_SUCCESS.
  /// \return Returns PLASMA_SUCCESS if the visitor returned true for a file.
  plResult FindFile(plDelegate<bool(const plDataDirPath&, const plFileStatus&)> visitor) const;

  ///@}
  /// \name File / Folder Updates
  ///@{

  /// \brief Force checking the filesystem for changes to the given file or folder.
  /// This function will handle file add/remove/change as well as folder add/remove. If an existing folder should be checked for changes, use CheckFolder instead.
  /// \param sAbsolutePath File or folder to check for changes.
  void NotifyOfChange(plStringView sAbsolutePath);

  /// \brief Check an existing folder recursively for changes.
  /// \param sAbsolutePath Absolute path to an existing folder in the model.
  void CheckFolder(plStringView sAbsolutePath);

  /// \brief Updates all files and folders in the model by iterating over all data directories. This is very expensive and should be done on a worker thread.
  void CheckFileSystem();

  ///@}
  /// \name File Meta Operations
  ///@{

  /// \brief Links a document Id to the given file. This allows for fast lookups whether a file is also a document.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param documentId The Id of the document that should be linked to the file.
  /// \return Returns PLASMA_SUCCESS if the file existed in the model and could be linked.
  plResult LinkDocument(plStringView sAbsolutePath, const plUuid& documentId);

  /// \brief Unlinks a document from a file
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \return Returns PLASMA_SUCCESS if the file existed.
  plResult UnlinkDocument(plStringView sAbsolutePath);

  /// \brief Creates a file reader to the given file. Will also link the document and hash it in a file-system-atomic operation.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param callback Called once the file was opened and hashed. The plFileStatus contains the up to date info for the file, including hash.
  /// \return Returns PLASMA_SUCCESS if the file existed and could be opened. Returns PLASMA_FAILURE if the file is not in the model or the file can't be opened for read access. On read failure, the file will be marked as locked.
  plResult ReadDocument(plStringView sAbsolutePath, const plDelegate<void(const plFileStatus&, plStreamReader&)>& callback);

  /// \brief Returns an up-to-date hash for the given file. Will trigger m_FileChangedEvents if the file has been modified since the last check. Hashes are cached so in the best case this will just check the timestamp on disk against the model and then return the cached hash. This function will also work on files outside of the data directories.
  /// \param sAbsolutePath Path to the document. Must be in the model.
  /// \param out_stat Contains the up to date info for the file, including hash.
  /// \return Returns PLASMA_SUCCESS if the file existed and could be opened. On failure, the file will be marked as locked.
  plResult HashFile(plStringView sAbsolutePath, plFileStatus& out_stat);

  ///@}

public:
  plCopyOnBroadcastEvent<const plFolderChangedEvent&, plMutex> m_FolderChangedEvents;
  plCopyOnBroadcastEvent<const plFileChangedEvent&, plMutex> m_FileChangedEvents;

private:
  void SetAllStatusUnknown();
  void RemoveStaleFileInfos();

  void OnAssetWatcherEvent(const plFileSystemWatcherEvent& e);
  plFileStatus HandleSingleFile(plDataDirPath absolutePath, bool bRecurseIntoFolders);
  plFileStatus HandleSingleFile(plDataDirPath absolutePath, const plFileStats& FileStat, bool bRecurseIntoFolders);

  void RemoveFileOrFolder(const plDataDirPath& absolutePath, bool bRecurseIntoFolders);

  void MarkFileLocked(plStringView sAbsolutePath);

  void FireFileChangedEvent(const plDataDirPath& file, plFileStatus fileStatus, plFileChangedEvent::Type type);
  void FireFolderChangedEvent(const plDataDirPath& file, plFolderChangedEvent::Type type);

private:
  // Immutable data after Initialize
  plApplicationFileSystemConfig m_FileSystemConfig;
  plDynamicArray<plString> m_DataDirRoots;
  plUniquePtr<plFileSystemWatcher> m_pWatcher;
  plEventSubscriptionID m_WatcherSubscription = {};

  // Actual file system data
  mutable plMutex m_FilesMutex;
  plAtomicBool m_bInitialized = false;

  FilesMap m_ReferencedFiles;     // Absolute path to stat map
  FoldersMap m_ReferencedFolders; // Absolute path to status map
  plSet<plString> m_LockedFiles;
  plMap<plString, plFileStatus> m_TransiendFiles; // Absolute path to stat for files outside the data directories.
};

#endif
