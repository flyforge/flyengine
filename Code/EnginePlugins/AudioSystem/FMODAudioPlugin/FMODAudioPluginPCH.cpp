#include <FMODAudioPlugin/FMODAudioPluginPCH.h>

#include <FMODAudioPlugin/FMODAudioPluginDLL.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>

PLASMA_STATICLINK_LIBRARY(FMODAudioPlugin)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(FMODAudioPlugin_FMODAudioSingleton);
  PLASMA_STATICLINK_REFERENCE(FMODAudioPlugin_FMODAudioPluginStartup);

}
