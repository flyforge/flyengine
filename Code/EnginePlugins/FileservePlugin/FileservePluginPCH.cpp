#include <FileservePlugin/FileservePluginPCH.h>

PLASMA_STATICLINK_LIBRARY(FileservePlugin)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveClient);
  PLASMA_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveDataDir);
  PLASMA_STATICLINK_REFERENCE(FileservePlugin_Fileserver_ClientContext);
  PLASMA_STATICLINK_REFERENCE(FileservePlugin_Fileserver_Fileserver);
  PLASMA_STATICLINK_REFERENCE(FileservePlugin_Main);
}
