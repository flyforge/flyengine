#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

PLASMA_STATICLINK_LIBRARY(OpenXRPlugin)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRSingleton);
  PLASMA_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRStartup);
  PLASMA_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRSpatialAnchors);
  PLASMA_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRHandTracking);
}
