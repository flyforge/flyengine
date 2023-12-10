#include <EditorPluginRecast/EditorPluginRecastPCH.h>

void OnLoadPlugin()
{
}

void OnUnloadPlugin() {}

PLASMA_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PLASMA_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
