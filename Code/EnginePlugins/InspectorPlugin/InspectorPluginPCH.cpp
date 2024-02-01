#include <InspectorPlugin/InspectorPluginPCH.h>

PL_STATICLINK_LIBRARY(InspectorPlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(InspectorPlugin_App);
  PL_STATICLINK_REFERENCE(InspectorPlugin_CVars);
  PL_STATICLINK_REFERENCE(InspectorPlugin_GlobalEvents);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Input);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Log);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Main);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Memory);
  PL_STATICLINK_REFERENCE(InspectorPlugin_OSFile);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Plugins);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Startup);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Stats);
  PL_STATICLINK_REFERENCE(InspectorPlugin_Time);
}
