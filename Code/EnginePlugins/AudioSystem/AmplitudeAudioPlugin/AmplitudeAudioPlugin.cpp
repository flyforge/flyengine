#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>

// BEGIN-DOCS-CODE-SNIPPET: plugin-setup
PL_PLUGIN_ON_LOADED()
{
  // you could do something here, though this is rare
}

PL_PLUGIN_ON_UNLOADED()
{
  // you could do something here, though this is rare
}
// END-DOCS-CODE-SNIPPET
