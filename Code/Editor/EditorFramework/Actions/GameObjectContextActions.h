#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectContextActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(const char* szMapping, const char* szPath);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hPickContextScene;
  static plActionDescriptorHandle s_hPickContextObject;
  static plActionDescriptorHandle s_hClearContextObject;
};

class PLASMA_EDITORFRAMEWORK_DLL plGameObjectContextAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectContextAction, plButtonAction);

public:
  enum class ActionType
  {
    PickContextScene,
    PickContextObject,
    ClearContextObject,
  };

  plGameObjectContextAction(const plActionContext& context, const char* szName, ActionType type);
  ~plGameObjectContextAction();

  virtual void Execute(const plVariant& value) override;

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);
  void Update();

  ActionType m_Type;
};
