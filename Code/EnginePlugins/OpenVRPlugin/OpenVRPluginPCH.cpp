#include <OpenVRPlugin/OpenVRPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <OpenVRPlugin/Basics.h>

PL_STATICLINK_LIBRARY(OpenVRPlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRSingleton);
  PL_STATICLINK_REFERENCE(OpenVRPlugin_OpenVRStartup);
}

PL_DYNAMIC_PLUGIN_IMPLEMENTATION(PL_OPENVRPLUGIN_DLL, plOpenVRPlugin);
