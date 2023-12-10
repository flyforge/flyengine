#pragma once

#include <FileservePlugin/FileservePluginDLL.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>

enum class plFileserveFileState
{
  None = 0,
  NonExistant = 1,
  NonExistantEither = 2,
  SameTimestamp = 3,
  SameHash = 4,
  Different = 5,
};

class PLASMA_FILESERVEPLUGIN_DLL plFileserveClientContext
{
public:
  struct DataDir
  {
    plString m_sRootName;
    plString m_sPathOnClient;
    plString m_sPathOnServer;
    plString m_sMountPoint;
    bool m_bMounted = false;
  };

  struct FileStatus
  {
    plInt64 m_iTimestamp = -1;
    plUInt64 m_uiHash = 0;
    plUInt64 m_uiFileSize = 0;
  };

  plFileserveFileState GetFileStatus(plUInt16& inout_uiDataDirID, const char* szRequestedFile, FileStatus& inout_Status,
    plDynamicArray<plUInt8>& out_FileContent, bool bForceThisDataDir) const;

  bool m_bLostConnection = false;
  plUInt32 m_uiApplicationID = 0;
  plHybridArray<DataDir, 8> m_MountedDataDirs;
};
