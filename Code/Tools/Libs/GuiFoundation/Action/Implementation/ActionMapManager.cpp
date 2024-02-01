#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>

plMap<plString, plActionMap*> plActionMapManager::s_Mappings;

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, ActionMapManager)

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

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plActionMapManager public functions
////////////////////////////////////////////////////////////////////////

plResult plActionMapManager::RegisterActionMap(plStringView sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (it.IsValid())
    return PL_FAILURE;

  s_Mappings.Insert(sMapping, PL_DEFAULT_NEW(plActionMap));
  return PL_SUCCESS;
}

plResult plActionMapManager::UnregisterActionMap(plStringView sMapping)
{
  auto it = s_Mappings.Find(sMapping);
  if (!it.IsValid())
    return PL_FAILURE;

  PL_DEFAULT_DELETE(it.Value());
  s_Mappings.Remove(it);
  return PL_SUCCESS;
}

plActionMap* plActionMapManager::GetActionMap(plStringView sMapping)
{
  auto it = s_Mappings.Find(sMapping);
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
  plDocumentActions::MapMenuActions("DocumentWindowTabMenu", "");
}

void plActionMapManager::Shutdown()
{
  plActionMapManager::UnregisterActionMap("DocumentWindowTabMenu").IgnoreResult();

  while (!s_Mappings.IsEmpty())
  {
    plResult res = UnregisterActionMap(s_Mappings.GetIterator().Key());
    PL_ASSERT_DEV(res == PL_SUCCESS, "Failed to call UnregisterActionMap successfully!");
    res.IgnoreResult();
  }
}
