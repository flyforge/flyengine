#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>

plMap<plString, plActionMap*> plActionMapManager::s_Mappings;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionMapManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plActionMapManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plActionMapManager::Shutdown();
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plActionMapManager public functions
////////////////////////////////////////////////////////////////////////

plResult plActionMapManager::RegisterActionMap(const char* szMapping)
{
  auto it = s_Mappings.Find(szMapping);
  if (it.IsValid())
    return PLASMA_FAILURE;

  s_Mappings.Insert(szMapping, PLASMA_DEFAULT_NEW(plActionMap));
  return PLASMA_SUCCESS;
}

plResult plActionMapManager::UnregisterActionMap(const char* szMapping)
{
  auto it = s_Mappings.Find(szMapping);
  if (!it.IsValid())
    return PLASMA_FAILURE;

  PLASMA_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
  return PLASMA_SUCCESS;
}

plActionMap* plActionMapManager::GetActionMap(const char* szMapping)
{
  auto it = s_Mappings.Find(szMapping);
  if (!it.IsValid())
    return nullptr;

  return it.Value();
}


////////////////////////////////////////////////////////////////////////
// plActionMapManager private functions
////////////////////////////////////////////////////////////////////////

void plActionMapManager::Startup()
{
  plActionMapManager::RegisterActionMap("DocumentWindowTabMenu").IgnoreResult();
  plDocumentActions::MapActions("DocumentWindowTabMenu", "", false);
}

void plActionMapManager::Shutdown()
{
  plActionMapManager::UnregisterActionMap("DocumentWindowTabMenu").IgnoreResult();

  while (!s_Mappings.IsEmpty())
  {
    plResult res = UnregisterActionMap(s_Mappings.GetIterator().Key());
    PLASMA_ASSERT_DEV(res == PLASMA_SUCCESS, "Failed to call UnregisterActionMap successfully!");
    res.IgnoreResult();
  }
}
