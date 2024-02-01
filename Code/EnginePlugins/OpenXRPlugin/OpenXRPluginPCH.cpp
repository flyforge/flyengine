#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>

PL_STATICLINK_LIBRARY(OpenXRPlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRSingleton);
  PL_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRStartup);
  PL_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRSpatialAnchors);
  PL_STATICLINK_REFERENCE(OpenXRPlugin_OpenXRHandTracking);
}
