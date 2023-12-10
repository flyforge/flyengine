#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

// A general problem when implementing a directory watcher is, that moving a folder out of the watched directory only communicates
// Which folder was moved (but not to where, nor its contents). This means when a folder is moved out of view,
// this needs to be treated as a delete. At the point of the move, it is no longer possible to query the contents of the folder.
// So a in memory copy of the file system is required in order to correctly implement a directory watcher.
template <typename T>
class plFileSystemMirror
{
public:
  enum class Type
  {
    File,
    Directory
  };

  struct DirEntry
  {
    plMap<plString, DirEntry> m_subDirectories;
    plMap<plString, T> m_files;
  };

  plFileSystemMirror();
  ~plFileSystemMirror();

  // \brief Adds the directory, and all files in it recursively.
  plResult AddDirectory(plStringView sPath, bool* out_pDirectoryExistsAlready = nullptr);

  // \brief Adds a file. Creates directories if they do not exist.
  plResult AddFile(plStringView sPath, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue);

  // \brief Removes a file.
  plResult RemoveFile(plStringView sPath);

  // \brief Removes a directory. Deletes any files & directories inside.
  plResult RemoveDirectory(plStringView sPath);

  // \brief Moves a directory. Any files & folders inside are moved with it.
  plResult MoveDirectory(plStringView sFromPath, plStringView sToPath);

  using EnumerateFunc = plDelegate<void(const plStringBuilder& path, Type type)>;

  // \brief Enumerates the files & directories under the given path
  plResult Enumerate(plStringView sPath, EnumerateFunc callbackFunc);

private:
  DirEntry* FindDirectory(plStringBuilder& path);

private:
  DirEntry m_TopLevelDir;
  plString m_sTopLevelDirPath;
};

namespace
{
  void EnsureTrailingSlash(plStringBuilder& ref_sBuilder)
  {
    if (!ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Append("/");
    }
  }

  void RemoveTrailingSlash(plStringBuilder& ref_sBuilder)
  {
    if (ref_sBuilder.EndsWith("/"))
    {
      ref_sBuilder.Shrink(0, 1);
    }
  }
} // namespace

template <typename T>
plFileSystemMirror<T>::plFileSystemMirror() = default;

template <typename T>
plFileSystemMirror<T>::~plFileSystemMirror() = default;

template <typename T>
plResult plFileSystemMirror<T>::AddDirectory(plStringView sPath, bool* out_pDirectoryExistsAlready)
{
  plStringBuilder currentDirAbsPath = sPath;
  currentDirAbsPath.MakeCleanPath();
  EnsureTrailingSlash(currentDirAbsPath);

  if (m_sTopLevelDirPath.IsEmpty())
  {
    m_sTopLevelDirPath = currentDirAbsPath;
    currentDirAbsPath.Shrink(0, 1); // remove trailing /

    DirEntry* currentDir = &m_TopLevelDir;

    plHybridArray<DirEntry*, 16> m_dirStack;

    plFileSystemIterator files;
    files.StartSearch(currentDirAbsPath.GetData(), plFileSystemIteratorFlags::ReportFilesAndFoldersRecursive);
    for (; files.IsValid(); files.Next())
    {
      const plFileStats& stats = files.GetStats();

      // In case we are done with a directory, move back up
      while (currentDirAbsPath != stats.m_sParentPath)
      {
        PLASMA_ASSERT_DEV(m_dirStack.GetCount() > 0, "Unexpected file iteration order");
        currentDir = m_dirStack.PeekBack();
        m_dirStack.PopBack();
        currentDirAbsPath.PathParentDirectory();
        RemoveTrailingSlash(currentDirAbsPath);
      }

      if (stats.m_bIsDirectory)
      {
        m_dirStack.PushBack(currentDir);
        plStringBuilder subdirName = stats.m_sName;
        EnsureTrailingSlash(subdirName);
        auto insertIt = currentDir->m_subDirectories.Insert(subdirName, DirEntry());
        currentDir = &insertIt.Value();
        currentDirAbsPath.AppendPath(stats.m_sName);
      }
      else
      {
        currentDir->m_files.Insert(std::move(stats.m_sName), T{});
      }
    }
    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = false;
    }
  }
  else
  {
    DirEntry* parentDir = FindDirectory(currentDirAbsPath);
    if (parentDir == nullptr)
    {
      return PLASMA_FAILURE;
    }

    if (out_pDirectoryExistsAlready != nullptr)
    {
      *out_pDirectoryExistsAlready = currentDirAbsPath.IsEmpty();
    }

    while (!currentDirAbsPath.IsEmpty())
    {
      const char* dirEnd = currentDirAbsPath.FindSubString("/");
      plStringView subdirName(currentDirAbsPath.GetData(), dirEnd + 1);
      auto insertIt = parentDir->m_subDirectories.Insert(subdirName, DirEntry());
      parentDir = &insertIt.Value();
      currentDirAbsPath.Shrink(plStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    }
  }

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plFileSystemMirror<T>::AddFile(plStringView sPath0, const T& value, bool* out_pFileExistsAlready, T* out_pOldValue)
{
  plStringBuilder sPath = sPath0;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return PLASMA_FAILURE; // file not under top level directory
  }

  const char* szSlashPos = sPath.FindSubString("/");

  while (szSlashPos != nullptr)
  {
    plStringView subdirName(sPath.GetData(), szSlashPos + 1);
    auto insertIt = dir->m_subDirectories.Insert(subdirName, DirEntry());
    dir = &insertIt.Value();
    sPath.Shrink(plStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()), 0);
    szSlashPos = sPath.FindSubString("/");
  }

  auto it = dir->m_files.Find(sPath);
  // Do not add the file twice
  if (!it.IsValid())
  {
    dir->m_files.Insert(sPath, value);
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = false;
    }
  }
  else
  {
    if (out_pFileExistsAlready != nullptr)
    {
      *out_pFileExistsAlready = true;
    }
    if (out_pOldValue != nullptr)
    {
      *out_pOldValue = it.Value();
    }
    it.Value() = value;
  }
  return PLASMA_SUCCESS;
}

template <typename T>
plResult plFileSystemMirror<T>::RemoveFile(plStringView sPath0)
{
  plStringBuilder sPath = sPath0;
  DirEntry* dir = FindDirectory(sPath);
  if (dir == nullptr)
  {
    return PLASMA_FAILURE; // file not under top level directory
  }

  if (sPath.FindSubString("/") != nullptr)
  {
    return PLASMA_FAILURE; // file does not exist
  }

  if (dir->m_files.GetCount() == 0)
  {
    return PLASMA_FAILURE; // there are no files in this directory
  }

  auto it = dir->m_files.Find(sPath);
  if (!it.IsValid())
  {
    return PLASMA_FAILURE; // file does not exist
  }

  dir->m_files.Remove(it);
  return PLASMA_SUCCESS;
}

template <typename T>
plResult plFileSystemMirror<T>::RemoveDirectory(plStringView sPath)
{
  plStringBuilder parentPath = sPath;
  plStringBuilder dirName = sPath;
  parentPath.PathParentDirectory();
  EnsureTrailingSlash(parentPath);
  dirName.Shrink(parentPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(dirName);

  DirEntry* parentDir = FindDirectory(parentPath);
  if (parentDir == nullptr || !parentPath.IsEmpty())
  {
    return PLASMA_FAILURE;
  }

  if (!parentDir->m_subDirectories.Remove(dirName))
  {
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

template <typename T>
plResult plFileSystemMirror<T>::MoveDirectory(plStringView sFromPath0, plStringView sToPath0)
{
  plStringBuilder sFromPath = sFromPath0;
  plStringBuilder sFromName = sFromPath0;
  sFromPath.PathParentDirectory();
  EnsureTrailingSlash(sFromPath);
  sFromName.Shrink(sFromPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sFromName);


  plStringBuilder sToPath = sToPath0;
  plStringBuilder sToName = sToPath0;
  sToPath.PathParentDirectory();
  EnsureTrailingSlash(sToPath);
  sToName.Shrink(sToPath.GetCharacterCount(), 0);
  EnsureTrailingSlash(sToName);

  DirEntry* moveFromDir = FindDirectory(sFromPath);
  if (!moveFromDir)
  {
    return PLASMA_FAILURE;
  }
  PLASMA_ASSERT_DEV(sFromPath.IsEmpty(), "move from directory should fully exist");

  DirEntry* moveToDir = FindDirectory(sToPath);
  if (!moveToDir)
  {
    return PLASMA_FAILURE;
  }

  if (!sToPath.IsEmpty())
  {
    do
    {
      const char* dirEnd = sToPath.FindSubString("/");
      plStringView subdirName(sToPath.GetData(), dirEnd + 1);
      auto insertIt = moveToDir->m_subDirectories.Insert(subdirName, DirEntry());
      moveToDir = &insertIt.Value();
      sToPath.Shrink(0, plStringUtils::GetCharacterCount(subdirName.GetStartPointer(), subdirName.GetEndPointer()));
    } while (!sToPath.IsEmpty());
  }

  DirEntry movedDir;
  {
    auto fromIt = moveFromDir->m_subDirectories.Find(sFromName);
    if (!fromIt.IsValid())
    {
      return PLASMA_FAILURE;
    }

    movedDir = std::move(fromIt.Value());
    moveFromDir->m_subDirectories.Remove(fromIt);
  }

  moveToDir->m_subDirectories.Insert(sToName, std::move(movedDir));

  return PLASMA_SUCCESS;
}

namespace
{
  template <typename T>
  struct plDirEnumerateState
  {
    typename plFileSystemMirror<T>::DirEntry* dir;
    typename plMap<plString, typename plFileSystemMirror<T>::DirEntry>::Iterator subDirIt;
  };
} // namespace

template <typename T>
plResult plFileSystemMirror<T>::Enumerate(plStringView sPath0, EnumerateFunc callbackFunc)
{
  plHybridArray<plDirEnumerateState<T>, 16> dirStack;
  plStringBuilder sPath = sPath0;
  if (!sPath.EndsWith("/"))
  {
    sPath.Append("/");
  }
  DirEntry* dirToEnumerate = FindDirectory(sPath);
  if (dirToEnumerate == nullptr)
  {
    return PLASMA_FAILURE;
  }
  if (!sPath.IsEmpty())
  {
    return PLASMA_FAILURE; // requested folder to enumerate doesn't exist
  }
  DirEntry* currentDir = dirToEnumerate;
  typename plMap<plString, plFileSystemMirror::DirEntry>::Iterator currentSubDirIt = currentDir->m_subDirectories.GetIterator();
  sPath = sPath0;

  while (currentDir != nullptr)
  {
    if (currentSubDirIt.IsValid())
    {
      DirEntry* nextDir = &currentSubDirIt.Value();
      sPath.AppendPath(currentSubDirIt.Key());
      currentSubDirIt.Next();
      dirStack.PushBack({currentDir, currentSubDirIt});
      currentDir = nextDir;
    }
    else
    {
      plStringBuilder sFilePath;
      for (auto& file : currentDir->m_files)
      {
        sFilePath = sPath;
        sFilePath.AppendPath(file.Key());
        callbackFunc(sFilePath, Type::File);
      }

      if (currentDir != dirToEnumerate)
      {
        if (sPath.EndsWith("/") && sPath.GetElementCount() > 1)
        {
          sPath.Shrink(0, 1);
        }
        callbackFunc(sPath, Type::Directory);
      }

      if (dirStack.IsEmpty())
      {
        currentDir = nullptr;
      }
      else
      {
        currentDir = dirStack.PeekBack().dir;
        currentSubDirIt = dirStack.PeekBack().subDirIt;
        dirStack.PopBack();
        sPath.PathParentDirectory();
        if (sPath.GetElementCount() > 1 && sPath.EndsWith("/"))
        {
          sPath.Shrink(0, 1);
        }
      }
    }
  }

  return PLASMA_SUCCESS;
}

template <typename T>
typename plFileSystemMirror<T>::DirEntry* plFileSystemMirror<T>::FindDirectory(plStringBuilder& path)
{
  if (!path.StartsWith(m_sTopLevelDirPath))
  {
    return nullptr;
  }
  path.TrimWordStart(m_sTopLevelDirPath);

  DirEntry* currentDir = &m_TopLevelDir;

  bool found = false;
  do
  {
    found = false;
    for (auto& dir : currentDir->m_subDirectories)
    {
      if (path.StartsWith(dir.Key()))
      {
        currentDir = &dir.Value();
        path.TrimWordStart(dir.Key());
        path.TrimWordStart("/");
        found = true;
        break;
      }
    }
  } while (found);

  return currentDir;
}
