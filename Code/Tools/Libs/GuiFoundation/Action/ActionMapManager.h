#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>

/// \brief A central place for creating and retrieving action maps.
class PLASMA_GUIFOUNDATION_DLL plActionMapManager
{
public:
  /// \brief Adds a new action map with the given name. Returns PLASMA_FAILURE if the name was already used before.
  static plResult RegisterActionMap(plStringView sMapping);

  /// \brief Deletes the action map with the given name. Returns PLASMA_FAILURE, if no such map exists.
  static plResult UnregisterActionMap(plStringView sMapping);

  /// \brief Returns the action map with the given name, or nullptr, if it doesn't exist.
  static plActionMap* GetActionMap(plStringView sMapping);

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static plMap<plString, plActionMap*> s_Mappings;
};
