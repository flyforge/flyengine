#pragma once

#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/Implementation/DataDirType.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Time/Timestamp.h>

class plArchiveEntry;

namespace plDataDirectory
{
  class ArchiveReaderUncompressed;
  class ArchiveReaderZstd;
  class ArchiveReaderZip;

  class PLASMA_FOUNDATION_DLL ArchiveType : public plDataDirectoryType
  {
  public:
    ArchiveType();
    ~ArchiveType();

    static plDataDirectoryType* Factory(plStringView sDataDirectory, plStringView sGroup, plStringView sRootName, plFileSystem::DataDirUsage usage);

    virtual const plString128& GetRedirectedDataDirectoryPath() const override { return m_sRedirectedDataDirPath; }

  protected:
    virtual plDataDirectoryReader* OpenFileToRead(plStringView sFile, plFileShareMode::Enum FileShareMode, bool bSpecificallyThisDataDir) override;

    virtual void RemoveDataDirectory() override;

    virtual bool ExistsFile(plStringView sFile, bool bOneSpecificDataDir) override;

    virtual plResult GetFileStats(plStringView sFileOrFolder, bool bOneSpecificDataDir, plFileStats& out_Stats) override;

    virtual plResult InternalInitializeDataDirectory(plStringView sDirectory) override;

    virtual void OnReaderWriterClose(plDataDirectoryReaderWriterBase* pClosed) override;

    plString128 m_sRedirectedDataDirPath;
    plString32 m_sArchiveSubFolder;
    plTimestamp m_LastModificationTime;
    plArchiveReader m_ArchiveReader;

    plMutex m_ReaderMutex;
    plHybridArray<plUniquePtr<ArchiveReaderUncompressed>, 4> m_ReadersUncompressed;
    plHybridArray<ArchiveReaderUncompressed*, 4> m_FreeReadersUncompressed;

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
    plHybridArray<plUniquePtr<ArchiveReaderZstd>, 4> m_ReadersZstd;
    plHybridArray<ArchiveReaderZstd*, 4> m_FreeReadersZstd;
#endif
  };

  class PLASMA_FOUNDATION_DLL ArchiveReaderUncompressed : public plDataDirectoryReader
  {
    PLASMA_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderUncompressed);

  public:
    ArchiveReaderUncompressed(plInt32 iDataDirUserData);
    ~ArchiveReaderUncompressed();

    virtual plUInt64 Read(void* pBuffer, plUInt64 uiBytes) override;
    virtual plUInt64 GetFileSize() const override;

  protected:
    virtual plResult InternalOpen(plFileShareMode::Enum FileShareMode) override;
    virtual void InternalClose() override;

    friend class ArchiveType;

    plUInt64 m_uiUncompressedSize = 0;
    plUInt64 m_uiCompressedSize = 0;
    plRawMemoryStreamReader m_MemStreamReader;
  };

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  class PLASMA_FOUNDATION_DLL ArchiveReaderZstd : public ArchiveReaderUncompressed
  {
    PLASMA_DISALLOW_COPY_AND_ASSIGN(ArchiveReaderZstd);

  public:
    ArchiveReaderZstd(plInt32 iDataDirUserData);
    ~ArchiveReaderZstd();

    virtual plUInt64 Read(void* pBuffer, plUInt64 uiBytes) override;

  protected:
    virtual plResult InternalOpen(plFileShareMode::Enum FileShareMode) override;

    friend class ArchiveType;

    plCompressedStreamReaderZstd m_CompressedStreamReader;
  };
#endif


} // namespace plDataDirectory
