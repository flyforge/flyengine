#pragma once

#include <FileservePlugin/Client/FileserveClient.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>

namespace plDataDirectory
{
  class FileserveDataDirectoryReader : public FolderReader
  {
  public:
    FileserveDataDirectoryReader(plInt32 iDataDirUserData);

  protected:
    virtual plResult InternalOpen(plFileShareMode::Enum FileShareMode) override;
  };

  class FileserveDataDirectoryWriter : public FolderWriter
  {
  protected:
    virtual void InternalClose() override;
  };

  /// \brief A data directory type to handle access to files that are served from a network host.
  class PL_FILESERVEPLUGIN_DLL FileserveType : public FolderType
  {
  public:
    /// \brief The factory that can be registered at plFileSystem to create data directories of this type.
    static plDataDirectoryType* Factory(plStringView sDataDirectory, plStringView sGroup, plStringView sRootName, plFileSystem::DataDirUsage usage);

    /// \brief [internal] Makes sure the redirection config files are up to date and then reloads them.
    virtual void ReloadExternalConfigs() override;

    /// \brief [internal] Called by FileserveDataDirectoryWriter when it is finished to upload the written file to the server
    void FinishedWriting(FolderWriter* pWriter);

  protected:
    virtual plDataDirectoryReader* OpenFileToRead(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;
    virtual plDataDirectoryWriter* OpenFileToWrite(plStringView sFile, plFileShareMode::Enum FileShareMode) override;
    virtual plResult InternalInitializeDataDirectory(plStringView sDirectory) override;
    virtual void RemoveDataDirectory() override;
    virtual void DeleteFile(plStringView sFile) override;
    virtual bool ExistsFile(plStringView sFile, bool bOneSpecificDataDir) override;
    /// \brief Limitation: Fileserve does not handle folders, only files. If someone stats a folder, this will fail.
    virtual plResult GetFileStats(plStringView sFileOrFolder, bool bOneSpecificDataDir, plFileStats& out_Stats) override;
    virtual FolderReader* CreateFolderReader() const override;
    virtual FolderWriter* CreateFolderWriter() const override;

    plUInt16 m_uiDataDirID = 0xffff;
    plString128 m_sFileserveCacheMetaFolder;
  };
} // namespace plDataDirectory
