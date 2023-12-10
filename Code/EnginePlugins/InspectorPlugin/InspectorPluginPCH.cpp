#include <InspectorPlugin/InspectorPluginPCH.h>

PLASMA_STATICLINK_LIBRARY(InspectorPlugin)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_App);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_CVars);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_GlobalEvents);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Input);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Log);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Main);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Memory);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_OSFile);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Plugins);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Startup);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Stats);
  PLASMA_STATICLINK_REFERENCE(InspectorPlugin_Time);
}
