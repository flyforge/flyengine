#include <RenderDocPlugin/RenderDocPluginPCH.h>

#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <RenderDocPlugin/RenderDocPluginDLL.h>

PLASMA_STATICLINK_LIBRARY(RenderDocPlugin)
{
  if (bReturn)
    return;

  PLASMA_STATICLINK_REFERENCE(RenderDocPlugin_RenderDocSingleton);
}
