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

  static void MapMenuActions(const char* szMapping, const char* szPath);
  static void MapToolbarActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hGreyBoxingGizmo;
};
