#include <EditorPluginRecast/EditorPluginRecastPCH.h>

void OnLoadPlugin()
{
}

void OnUnloadPlugin() {}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}
