#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/ActionMap.h>


class PLASMA_GUIFOUNDATION_DLL plActionMapManager
{
public:
  static plResult RegisterActionMap(const char* szMapping);
  static plResult UnregisterActionMap(const char* szMapping);
  static plActionMap* GetActionMap(const char* szMapping);

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionMapManager);

  static void Startup();
  static void Shutdown();

private:
  static plMap<plString, plActionMap*> s_Mappings;
};
