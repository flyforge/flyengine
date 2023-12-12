#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/TransformGizmoActions.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <EditorPluginScene/EditTools/GreyBoxEditTool.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

plActionDescriptorHandle plSceneGizmoActions::s_hGreyBoxingGizmo;

void plSceneGizmoActions::RegisterActions()
{
  s_hGreyBoxingGizmo =
    PLASMA_REGISTER_ACTION_1("Gizmo.Mode.GreyBoxing", plActionScope::Document, "Gizmo", "B", plGizmoAction, plGetStaticRTTI<plGreyBoxEditTool>());
}

void plSceneGizmoActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hGreyBoxingGizmo);
}

void plSceneGizmoActions::MapMenuActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/Gizmo.Menu");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}

void plSceneGizmoActions::MapToolbarActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  plStringBuilder sSubPath(szPath, "/GizmoCategory");
  pMap->MapAction(s_hGreyBoxingGizmo, sSubPath, 5.0f);
}
