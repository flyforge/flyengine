#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/IO/FileEnums.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Time/Timestamp.h>

struct plOSFileData;

#if PLASMA_ENABLED(PLASMA_USE_POSIX_FILE_API)
#  include <Foundation/IO/Implementation/Posix/OSFileDeclarations_posix.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/IO/Implementation/Win/OSFileDeclarations_win.h>
#endif

/// \brief Defines in which mode to open a file.
struct plFileOpenMode
{
  enum Enum
  {
    None,   ///< None, only used internally.
    Read,   ///< Open file for reading.
    Write,  ///< Open file for writing (already existing data is discarded).
    Append, ///< Open file for appending (writing, but always only at the end, already existing data is preserved).
  };
};

/// \brief Holds the stats for a file.
struct PLASMA_FOUNDATION_DLL plFileStats
{
  plFileStats();
  ~plFileStats();

  /// \brief Stores the concatenated m_sParentPath and m_sName in \a path.
  void GetFullPath(plStringBuilder& ref_sPath) const;

  /// \brief Path to the parent folder.
  /// Append m_sName to m_sParentPath to obtain the full path.
  plStringBuilder m_sParentPath;

  /// \brief The name of the file or folder that the stats are for. Does not include the parent path to it.
  /// Append m_sName to m_sParentPath to obtain the full path.
  plString m_sName;

  /// \brief The last modification time as an UTC timestamp since Unix epoch.
  plTimestamp m_LastModificationTime;

  /// \brief The size of the file in bytes.
  plUInt64 m_uiFileSize = 0;

  /// \brief Whether the file object is a file or folder.
  bool m_bIsDirectory = false;
};

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS) || defined(PLASMA_DOCS)

struct plFileIterationData;

struct plFileSystemIteratorFlags
{
  using StorageType = plUInt8;

  enum Enum : plUInt8
  {
    Recursive = PLASMA_BIT(0),
    ReportFiles = PLASMA_BIT(1),
    ReportFolders = PLASMA_BIT(2),

    ReportFilesRecursive = Recursive | ReportFiles,
    ReportFoldersRecursive = Recursive | ReportFolders,
    ReportFilesAndFoldersRecursive = Recursive | ReportFiles | ReportFolders,

    Default = ReportFilesAndFoldersRecursive,
  };

  struct Bits
  {
    StorageType Recursive : 1;
    StorageType ReportFiles : 1;
    StorageType ReportFolders : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plFileSystemIteratorFlags);

/// \brief An plFileSystemIterator allows to iterate over all files in a certain directory.
///
/// The search can be recursive, and it can contain wildcards (* and ?) to limit the search to specific file types.
class PLASMA_FOUNDATION_DLL plFileSystemIterator
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plFileSystemIterator);

public:
  plFileSystemIterator();
  ~plFileSystemIterator();

  /// \brief Starts a search at the given folder. Use * and ? as wildcards.
  ///
  /// To iterate all files from one folder, use '/Some/Folder'
  /// To iterate over all files of a certain type (in one folder) use '/Some/Folder/*.ext'
  /// Only the final path segment can use placeholders, folders in between must be fully named.
  /// If bRecursive is false, the iterator will only iterate over the files in the start folder, and will not recurse into subdirectories.
  /// If bReportFolders is false, only files will be reported, folders will be skipped (though they will be recursed into, if bRecursive is true).
  ///
  /// If PLASMA_SUCCESS is returned, the iterator points to a valid file, and the functions GetCurrentPath() and GetStats() will return
  /// the information about that file. To advance to the next file, use Next() or SkipFolder().
  /// When no iteration is possible (the directory does not exist or the wild-cards are used incorrectly), PLASMA_FAILURE is returned.
  void StartSearch(plStringView sSearchTerm, plBitflags<plFileSystemIteratorFlags> flags = plFileSystemIteratorFlags::Default); // [tested]

  /// \brief The same as StartSearch() but executes the same search on multiple folders.
  ///
  /// The search term is appended to each start folder and they are searched one after the other.
  void StartMultiFolderSearch(plArrayPtr<plString> startFolders, plStringView sSearchTerm, plBitflags<plFileSystemIteratorFlags> flags = plFileSystemIteratorFlags::Default);

  /// \brief Returns the search string with which StartSearch() was called.
  ///
  /// If StartMultiFolderSearch() is used, every time a new top-level folder is entered, StartSearch() is executed. In this case GetCurrentSearchTerm() can be used to know in which top-level folder the search is currently running.
  const plStringView GetCurrentSearchTerm() const { return m_sSearchTerm; }

  /// \brief Returns the current path in which files are searched. Changes when 'Next' moves in or out of a sub-folder.
  ///
  /// You can use this to get the full path of the current file, by appending this value and the filename from 'GetStats'
  const plStringBuilder& GetCurrentPath() const { return m_sCurPath; } // [tested]

  /// \brief Returns the file stats of the current object that the iterator points to.
  const plFileStats& GetStats() const { return m_CurFile; } // [tested]

  /// \brief Advances the iterator to the next file object. Might recurse into sub-folders.
  void Next(); // [tested]

  /// \brief The same as 'Next' only that the current folder will not be recursed into.
  void SkipFolder(); // [tested]

  /// \brief Returns true if the iterator currently points to a valid file entry.
  bool IsValid() const;

private:
  plInt32 InternalNext();

  /// \brief The current path of the folder, in which the iterator currently is.
  plStringBuilder m_sCurPath;

  plBitflags<plFileSystemIteratorFlags> m_Flags;

  /// \brief The stats about the file that the iterator currently points to.
  plFileStats m_CurFile;

  /// \brief Platform specific data, required by the implementation.
  plFileIterationData m_Data;

  plString m_sSearchTerm;
  plString m_sMultiSearchTerm;
  plUInt32 m_uiCurrentStartFolder = 0;
  plHybridArray<plString, 8> m_StartFolders;
};

#endif

/// \brief This is an abstraction for the most important file operations.
///
/// Instances of plOSFile can be used for reading and writing files.
/// All paths must be absolute paths, relative paths and current working directories are not supported,
/// since that cannot be guaranteed to work equally on all platforms under all circumstances.
/// A few static functions allow to query the most important data about files, to delete files and create directories.
class PLASMA_FOUNDATION_DLL plOSFile
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plOSFile);

public:
  plOSFile();
  ~plOSFile();

  /// \brief Opens a file for reading or writing. Returns PLASMA_SUCCESS if the file could be opened successfully.
  plResult Open(plStringView sFile, plFileOpenMode::Enum openMode, plFileShareMode::Enum fileShareMode = plFileShareMode::Default); // [tested]

  /// \brief Returns true if a file is currently open.
  bool IsOpen() const; // [tested]

  /// \brief Closes the file, if it is currently opened.
  void Close(); // [tested]

  /// \brief Writes the given number of bytes from the buffer into the file. Returns true if all data was successfully written.
  plResult Write(const void* pBuffer, plUInt64 uiBytes); // [tested]

  /// \brief Reads up to the given number of bytes from the file. Returns the actual number of bytes that was read.
  plUInt64 Read(void* pBuffer, plUInt64 uiBytes); // [tested]

  /// \brief Reads the entire file content into the given array
  plUInt64 ReadAll(plDynamicArray<plUInt8>& out_fileContent); // [tested]

  /// \brief Returns the name of the file that is currently opened. Returns an empty string, if no file is open.
  plStringView GetOpenFileName() const { return m_sFileName; } // [tested]

  /// \brief Returns the position in the file at which read/write operations will occur.
  plUInt64 GetFilePosition() const; // [tested]

  /// \brief Sets the position where in the file to read/write next.
  void SetFilePosition(plInt64 iDistance, plFileSeekMode::Enum pos) const; // [tested]

  /// \brief Returns the current total size of the file.
  plUInt64 GetFileSize() const; // [tested]

  /// \brief This will return the platform specific file data (handles etc.), if you really want to be able to wreak havoc.
  const plOSFileData& GetFileData() const { return m_FileData; }

  /// \brief Returns the processes current working directory (CWD).
  ///
  /// The value typically depends on the directory from which the application was launched.
  /// Since this is a process wide global variable, other code can modify it at any time.
  ///
  /// \note pl does not use the CWD for any file resolution. This function is provided to enable
  /// tools to work with relative paths from the command-line, but every application has to implement
  /// such behavior individually.
  static const plString GetCurrentWorkingDirectory(); // [tested]

  /// \brief If szPath is a relative path, this function prepends GetCurrentWorkingDirectory().
  ///
  /// In either case, MakeCleanPath() is used before the string is returned.
  static const plString MakePathAbsoluteWithCWD(plStringView sPath); // [tested]

  /// \brief Checks whether the given file exists.
  static bool ExistsFile(plStringView sFile); // [tested]

  /// \brief Checks whether the given file exists.
  static bool ExistsDirectory(plStringView sDirectory); // [tested]

  /// \brief If the given file already exists, determines a file path that doesn't exist yet.
  ///
  /// If the original file already exists, sSuffix is appended and then a number starting at 1.
  /// Loops until it finds a filename that is not yet taken.
  static void FindFreeFilename(plStringBuilder& inout_sPath, plStringView sSuffix = "-");

  /// \brief Deletes the given file. Returns PLASMA_SUCCESS, if the file was deleted or did not exist in the first place. Returns PLASMA_FAILURE
  static plResult DeleteFile(plStringView sFile); // [tested]

  /// \brief Creates the given directory structure (meaning all directories in the path, that do not exist). Returns false, if any directory could not
  /// be created.
  static plResult CreateDirectoryStructure(plStringView sDirectory); // [tested]

  /// \brief Renames / Moves an existing directory. The file / directory at szFrom must exist. The parent directory of szTo must exist.
  /// Returns PLASMA_FAILURE if the move failed.
  static plResult MoveFileOrDirectory(plStringView sFrom, plStringView sTo);

  /// \brief Copies the source file into the destination file.
  static plResult CopyFile(plStringView sSource, plStringView sDestination); // [tested]

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS) || defined(PLASMA_DOCS)
  /// \brief Gets the stats about the given file or folder. Returns false, if the stats could not be determined.
  static plResult GetFileStats(plStringView sFileOrFolder, plFileStats& out_stats); // [tested]

#  if (PLASMA_ENABLED(PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS) && PLASMA_ENABLED(PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS)) || defined(PLASMA_DOCS)
  /// \brief Useful on systems that are not strict about the casing of file names. Determines the correct name of a file.
  static plResult GetFileCasing(plStringView sFileOrFolder, plStringBuilder& out_sCorrectSpelling); // [tested]
#  endif

#endif

#if (PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS) && PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)) || defined(PLASMA_DOCS)

  /// \brief Returns the plFileStats for all files and folders in the given folder
  static void GatherAllItemsInFolder(plDynamicArray<plFileStats>& out_itemList, plStringView sFolder, plBitflags<plFileSystemIteratorFlags> flags = plFileSystemIteratorFlags::Default);

  /// \brief Copies \a szSourceFolder to \a szDestinationFolder. Overwrites existing files.
  ///
  /// If \a out_FilesCopied is provided, the destination path of every successfully copied file is appended to it.
  static plResult CopyFolder(plStringView sSourceFolder, plStringView sDestinationFolder, plDynamicArray<plString>* out_pFilesCopied = nullptr);

  /// \brief Deletes all files recursively in \a szFolder.
  ///
  /// \note The current implementation does not remove the (empty) folders themselves.
  static plResult DeleteFolder(plStringView sFolder);

#endif

  /// \brief Returns the path in which the applications binary file is located.
  static plStringView GetApplicationDirectory();

  /// \brief Returns the folder into which user data may be safely written.
  /// Append a sub-folder for your application.
  ///
  /// On Windows this is the '%appdata%' directory.
  /// On Posix systems this is the '~' (home) directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static plString GetUserDataFolder(plStringView sSubFolder = {});

  /// \brief Returns the folder into which temp data may be written.
  ///
  /// On Windows this is the '%localappdata%/Temp' directory.
  /// On Posix systems this is the '~/.cache' directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static plString GetTempDataFolder(plStringView sSubFolder = {});

  /// \brief Returns the folder into which the user may want to store documents.
  /// Append a sub-folder for your application.
  ///
  /// On Windows this is the 'Documents' directory.
  /// On Posix systems this is the '~' (home) directory.
  ///
  /// If szSubFolder is specified, it will be appended to the result.
  static plString GetUserDocumentsFolder(plStringView sSubFolder = {});


public:
  /// \brief Describes the types of events that plOSFile sends.
  struct EventType
  {
    enum Enum
    {
      None,
      FileOpen,        ///< A file has been (attempted) to open.
      FileClose,       ///< An open file has been closed.
      FileExists,      ///< A check whether a file exists has been done.
      DirectoryExists, ///< A check whether a directory exists has been done.
      FileDelete,      ///< A file was attempted to be deleted.
      FileRead,        ///< From an open file data was read.
      FileWrite,       ///< Data was written to an open file.
      MakeDir,         ///< A path has been created (recursive directory creation).
      FileCopy,        ///< A file has been copied to another location.
      FileStat,        ///< The stats of a file are queried
      FileCasing,      ///< The exact spelling of a file/path is requested
    };
  };

  /// \brief The data that is sent through the event interface.
  struct EventData
  {
    /// \brief The type of information that is sent.
    EventType::Enum m_EventType = EventType::None;

    /// \brief A unique ID for each file access. Reads and writes to the same open file use the same ID. If the same file is opened multiple times,
    /// different IDs are used.
    plInt32 m_iFileID = 0;

    /// \brief The name of the file that was operated upon.
    plStringView m_sFile;

    /// \brief If a second file was operated upon (FileCopy), that is the second file name.
    plStringView m_sFile2;

    /// \brief Mode that a file has been opened in.
    plFileOpenMode::Enum m_FileMode = plFileOpenMode::None;

    /// \brief Whether the operation succeeded (reading, writing, etc.)
    bool m_bSuccess = true;

    /// \brief How long the operation took.
    plTime m_Duration;

    /// \brief How many bytes were transfered (reading, writing)
    plUInt64 m_uiBytesAccessed = 0;
  };

  using Event = plEvent<const EventData&, plMutex>;

  /// \brief Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddEventHandler(Event::Handler handler) { s_FileEvents.AddEventHandler(handler); }

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveEventHandler(Event::Handler handler) { s_FileEvents.RemoveEventHandler(handler); }

private:
  /// \brief Manages all the Event Handlers for the OSFile events.
  static Event s_FileEvents;

  // *** Internal Functions that do the platform specific work ***

  plResult InternalOpen(plStringView sFile, plFileOpenMode::Enum OpenMode, plFileShareMode::Enum FileShareMode);
  void InternalClose();
  plResult InternalWrite(const void* pBuffer, plUInt64 uiBytes);
  plUInt64 InternalRead(void* pBuffer, plUInt64 uiBytes);
  plUInt64 InternalGetFilePosition() const;
  void InternalSetFilePosition(plInt64 iDistance, plFileSeekMode::Enum Pos) const;

  static bool InternalExistsFile(plStringView sFile);
  static bool InternalExistsDirectory(plStringView sDirectory);
  static plResult InternalDeleteFile(plStringView sFile);
  static plResult InternalDeleteDirectory(plStringView sDirectory);
  static plResult InternalCreateDirectory(plStringView sFile);
  static plResult InternalMoveFileOrDirectory(plStringView sDirectoryFrom, plStringView sDirectoryTo);

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)
  static plResult InternalGetFileStats(plStringView sFileOrFolder, plFileStats& out_Stats);
#endif

  // *************************************************************

  /// \brief Stores the mode with which the file was opened.
  plFileOpenMode::Enum m_FileMode;

  /// [internal] On win32 when a file is already open, and this is true, plOSFile will wait until the file becomes available
  bool m_bRetryOnSharingViolation = true;

  /// \brief Stores the (cleaned up) filename that was used to open the file.
  plStringBuilder m_sFileName;

  /// \brief Stores the value of s_FileCounter when the plOSFile is created.
  plInt32 m_iFileID;

  /// \brief Platform specific data about the open file.
  plOSFileData m_FileData;

  /// \brief The application binaries' path.
  static plString64 s_sApplicationPath;

  /// \brief The path where user data is stored on this OS
  static plString64 s_sUserDataPath;

  /// \brief The path where temp data is stored on this OS
  static plString64 s_sTempDataPath;

  /// \brief The path where user data documents are stored on this OS
  static plString64 s_sUserDocumentsPath;

  /// \brief Counts how many different files are touched.225
  static plAtomicInteger32 s_iFileCounter;
};
