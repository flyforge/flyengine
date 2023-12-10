#pragma once

#include <EditorPluginScene/EditorPluginSceneDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/////
class PLASMA_EDITORPLUGINSCENE_DLL plSceneGizmoActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(plStringView sMapping);
  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hGreyBoxingGizmo;
};
