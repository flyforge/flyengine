#include <RenderDocPlugin/RenderDocPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <RenderDocPlugin/RenderDocPluginDLL.h>

PL_STATICLINK_LIBRARY(RenderDocPlugin)
{
  if (bReturn)
    return;

  PL_STATICLINK_REFERENCE(RenderDocPlugin_RenderDocSingleton);
}
