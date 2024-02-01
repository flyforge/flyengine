#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

plActionDescriptorHandle plSceneGizmoActions::s_hGreyBoxingGizmo;

void plSceneGizmoActions::RegisterActions()
{
  s_hGreyBoxingGizmo = PL_REGISTER_ACTION_1("Gizmo.Mode.GreyBoxing", plActionScope::Document, "Gizmo", "B", plGizmoAction, plGetStaticRTTI<plGreyBoxEditTool>());
}

void plSceneGizmoActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hGreyBoxingGizmo);
}

void plSceneGizmoActions::MapMenuActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hGreyBoxingGizmo, "G.Gizmos", 5.0f);
}

void plSceneGizmoActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  const plStringView sSubPath("GizmoCategory");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}
