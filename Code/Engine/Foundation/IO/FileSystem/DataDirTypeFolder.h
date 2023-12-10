#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/OSFile.h>

namespace plDataDirectory
{
  class FolderReader;
  class FolderWriter;

  /// \brief A data directory type to handle access to ordinary files.
  ///
  /// Register the 'Factory' function at plFileSystem to allow it to mount local directories.
  class PLASMA_FOUNDATION_DLL FolderType : public plDataDirectoryType
  {
  public:
    ~FolderType();

    /// \brief The factory that can be registered at plFileSystem to create data directories of this type.
    static plDataDirectoryType* Factory(plStringView sDataDirectory, plStringView sGroup, plStringView sRootName, plFileSystem::DataDirUsage usage);

    /// A 'redirection file' is an optional file located inside a data directory that lists which file access is redirected to which other
    /// file lookup. Each redirection is one line in the file (terminated by a \n). Each line consists of the 'key' string, a semicolon and
    /// a 'value' string. No unnecessary whitespace is allowed. When a file that matches 'key' is accessed through a mounted data directory,
    /// the file access will be replaced by 'value' (plus s_sRedirectionPrefix) 'key' may be anything (e.g. a GUID string), 'value' should
    /// be a valid relative path into the SAME data directory. The redirection file can be used to implement an asset lookup, where assets
    /// are identified by GUIDs and need to be mapped to the actual asset file.
    static plString s_sRedirectionFile;

    /// If a redirection file is used AND the redirection lookup was successful, s_sRedirectionPrefix is prepended to the redirected file
    /// access.
    static plString s_sRedirectionPrefix;

    /// \brief When s_sRedirectionFile and s_sRedirectionPrefix are used to enable file redirection, this will reload those config files.
    virtual void ReloadExternalConfigs() override;

    virtual const plString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    // The implementations of the abstract functions.

    virtual plDataDirectoryReader* OpenFileToRead(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual bool ResolveAssetRedirection(plStringView sPathOrAssetGuid, plStringBuilder& out_sRedirection) override;
    virtual plDataDirectoryWriter* OpenFileToWrite(plStringView sFile, plFileShareMode::Enum FileShareMode) override;
    virtual void RemoveDataDirectory() override;
    virtual void DeleteFile(plStringView sFile) override;
    virtual bool ExistsFile(plStringView sFile, bool bOneSpecificDataDir) override;
    virtual plResult GetFileStats(plStringView sFileOrFolder, bool bOneSpecificDataDir, plFileStats& out_Stats) override;
    virtual FolderReader* CreateFolderReader() const;
    virtual FolderWriter* CreateFolderWriter() const;

    /// \brief Called by 'plDataDirectoryType_Folder::Factory'
    virtual plResult InternalInitializeDataDirectory(plStringView sDirectory) override;

    /// \brief Marks the given reader/writer as reusable.
    virtual void OnReaderWriterClose(plDataDirectoryReaderWriterBase* pClosed) override;

    void LoadRedirectionFile();

    mutable plMutex m_ReaderWriterMutex; ///< Locks m_Readers / m_Writers as well as the m_bIsInUse flag of each reader / writer.
    plHybridArray<plDataDirectory::FolderReader*, 4> m_Readers;
    plHybridArray<plDataDirectory::FolderWriter*, 4> m_Writers;

    mutable plMutex m_RedirectionMutex;
    plMap<plString, plString> m_FileRedirection;
    plString128 m_sRedirectedDataDirPath;
  };


  /// \brief Handles reading from ordinary files.
  class PLASMA_FOUNDATION_DLL FolderReader : public plDataDirectoryReader
  {
    PLASMA_DISALLOW_COPY_AND_ASSIGN(FolderReader);

  public:
    FolderReader(plInt32 iDataDirUserData)
      : plDataDirectoryReader(iDataDirUserData)
    {
      m_bIsInUse = false;
    }

    virtual plUInt64 Read(void* pBuffer, plUInt64 uiBytes) override;
    virtual plUInt64 GetFileSize() const override;

  protected:
    virtual plResult InternalOpen(plFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    friend class FolderType;

    bool m_bIsInUse;
    plOSFile m_File;
  };

  /// \brief Handles writing to ordinary files.
  class PLASMA_FOUNDATION_DLL FolderWriter : public plDataDirectoryWriter
  {
    PLASMA_DISALLOW_COPY_AND_ASSIGN(FolderWriter);

  public:
    FolderWriter(plInt32 iDataDirUserData = 0)
      : plDataDirectoryWriter(iDataDirUserData)
    {
      m_bIsInUse = false;
    }

    virtual plResult Write(const void* pBuffer, plUInt64 uiBytes) override;
    virtual plUInt64 GetFileSize() const override;

  protected:
    virtual plResult InternalOpen(plFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    friend class FolderType;

    bool m_bIsInUse;
    plOSFile m_File;
  };
} // namespace plDataDirectory
