#include <FileservePlugin/FileservePluginPCH.h>

PL_STATICLINK_LIBRARY(FileservePlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveClient);
  PL_STATICLINK_REFERENCE(FileservePlugin_Client_FileserveDataDir);
  PL_STATICLINK_REFERENCE(FileservePlugin_Fileserver_ClientContext);
  PL_STATICLINK_REFERENCE(FileservePlugin_Fileserver_Fileserver);
  PL_STATICLINK_REFERENCE(FileservePlugin_Main);
}
