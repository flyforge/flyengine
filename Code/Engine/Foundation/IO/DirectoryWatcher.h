#pragma once

#include <Foundation/Basics.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>
#  include <Foundation/Types/Bitflags.h>
#  include <Foundation/Types/Delegate.h>

struct plDirectoryWatcherImpl;

/// \brief Which action has been performed on a file.
enum class plDirectoryWatcherAction
{
  None,           ///< Nothing happend
  Added,          ///< A file or directory was added
  Removed,        ///< A file or directory was removed
  Modified,       ///< A file was modified. Both Reads and Writes can 'modify' the timestamps of a file.
  RenamedOldName, ///< A file or directory was renamed. First the old name is provided.
  RenamedNewName, ///< A file or directory was renamed. The new name is provided second.
};

enum class plDirectoryWatcherType
{
  File,
  Directory
};

/// \brief
///   Watches file actions in a directory. Changes need to be polled.
class PLASMA_FOUNDATION_DLL plDirectoryWatcher
{
public:
  /// \brief What to watch out for.
  struct Watch
  {
    using StorageType = plUInt8;
    constexpr static plUInt8 Default = 0;

    /// \brief Enum values
    enum Enum
    {
      Writes = PLASMA_BIT(0),         ///< Watch for writes.
      Creates = PLASMA_BIT(1),        ///< Watch for newly created files.
      Deletes = PLASMA_BIT(2),        ///< Watch for deleted files.
      Renames = PLASMA_BIT(3),        ///< Watch for renames.
      Subdirectories = PLASMA_BIT(4), ///< Watch files in subdirectories recursively.
    };

    struct Bits
    {
      StorageType Writes : 1;
      StorageType Creates : 1;
      StorageType Deletes : 1;
      StorageType Renames : 1;
      StorageType Subdirectories : 1;
    };
  };

  plDirectoryWatcher();
  ~plDirectoryWatcher();

  /// \brief
  ///   Opens the directory at \p absolutePath for watching. \p whatToWatch controls what exactly should be watched.
  ///
  /// \note A instance of plDirectoryWatcher can only watch one directory at a time.
  plResult OpenDirectory(plStringView sAbsolutePath, plBitflags<Watch> whatToWatch);

  /// \brief
  ///   Closes the currently watched directory if any.
  void CloseDirectory();

  /// \brief
  ///   Returns the opened directory, will be empty if no directory was opened.
  plStringView GetDirectory() const { return m_sDirectoryPath; }

  using EnumerateChangesFunction = plDelegate<void(plStringView sFilename, plDirectoryWatcherAction action, plDirectoryWatcherType type), 48>;

  /// \brief
  ///   Calls the callback \p func for each change since the last call. For each change the filename
  ///   and the action, which was performed on the file, is passed to \p func.
  ///   If waitUpToMilliseconds is greater than 0, blocks until either a change was observed or the timelimit is reached.
  ///
  /// \note There might be multiple changes on the same file reported.
  void EnumerateChanges(EnumerateChangesFunction func, plTime waitUpTo = plTime::Zero());

  /// \brief
  ///   Same as the other EnumerateChanges function, but enumerates multiple watchers.
  static void EnumerateChanges(plArrayPtr<plDirectoryWatcher*> watchers, EnumerateChangesFunction func, plTime waitUpTo = plTime::Zero());

private:
  plString m_sDirectoryPath;
  plDirectoryWatcherImpl* m_pImpl = nullptr;
};

PLASMA_DECLARE_FLAGS_OPERATORS(plDirectoryWatcher::Watch);

#endif
