#include <FileservePlugin/FileservePluginPCH.h>

#include <FileservePlugin/Fileserver/ClientContext.h>
#include <Foundation/IO/FileSystem/FileReader.h>

plFileserveFileState plFileserveClientContext::GetFileStatus(plUInt16& inout_uiDataDirID, const char* szRequestedFile, FileStatus& inout_Status,
  plDynamicArray<plUInt8>& out_FileContent, bool bForceThisDataDir) const
{
  for (plUInt16 i = static_cast<plUInt16>(m_MountedDataDirs.GetCount()); i > 0; --i)
  {
    const plUInt16 uiDataDirID = i - 1;

    // when !bForceThisDataDir search for the best match
    // otherwise lookup this exact data dir
    if (bForceThisDataDir && uiDataDirID != inout_uiDataDirID)
      continue;

    const auto& dd = m_MountedDataDirs[uiDataDirID];

    if (!dd.m_bMounted)
      continue;

    plStringBuilder sAbsPath;
    sAbsPath = dd.m_sPathOnServer;
    sAbsPath.AppendPath(szRequestedFile);

    plFileStats stat;
    if (plOSFile::GetFileStats(sAbsPath, stat).Failed())
      continue;

    inout_Status.m_uiFileSize = stat.m_uiFileSize;

    const plInt64 iNewTimestamp = stat.m_LastModificationTime.GetInt64(plSIUnitOfTime::Microsecond);

    if (inout_Status.m_iTimestamp == iNewTimestamp && inout_uiDataDirID == uiDataDirID)
      return plFileserveFileState::SameTimestamp;

    inout_Status.m_iTimestamp = iNewTimestamp;

    // read the entire file
    {
      plFileReader file;
      if (file.Open(sAbsPath).Failed())
        continue;

      plUInt64 uiNewHash = 1;
      out_FileContent.SetCountUninitialized((plUInt32)inout_Status.m_uiFileSize);

      if (!out_FileContent.IsEmpty())
      {
        file.ReadBytes(out_FileContent.GetData(), out_FileContent.GetCount());
        uiNewHash = plHashingUtils::xxHash64(out_FileContent.GetData(), (size_t)out_FileContent.GetCount(), uiNewHash);

        // if the file is empty, the hash will be zero, which could lead to an incorrect assumption that the hash is the same
        // instead always transfer the empty file, so that it properly exists on the client
        if (inout_Status.m_uiHash == uiNewHash && inout_uiDataDirID == uiDataDirID)
          return plFileserveFileState::SameHash;
      }

      inout_Status.m_uiHash = uiNewHash;
    }

    inout_uiDataDirID = uiDataDirID;
    return plFileserveFileState::Different;
  }

  inout_Status.m_iTimestamp = 0;
  inout_Status.m_uiFileSize = 0;
  inout_Status.m_uiHash = 0;

  // the client doesn't have the file either
  // this is an optimization to prevent redundant file deletions on the client
  if (inout_Status.m_iTimestamp == 0 && inout_Status.m_uiHash == 0)
    return plFileserveFileState::NonExistantEither;

  return plFileserveFileState::NonExistant;
}



PLASMA_STATICLINK_FILE(FileservePlugin, FileservePlugin_Fileserver_ClientContext);
