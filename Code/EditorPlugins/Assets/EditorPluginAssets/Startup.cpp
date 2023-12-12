#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Foundation/Configuration/Startup.h>

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(Editor, PluginAssets)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on
