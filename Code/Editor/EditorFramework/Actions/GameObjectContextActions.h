#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class PL_EDITORFRAMEWORK_DLL plGameObjectContextActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(plStringView sMapping);
  static void MapContextMenuActions(plStringView sMapping);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hPickContextScene;
  static plActionDescriptorHandle s_hPickContextObject;
  static plActionDescriptorHandle s_hClearContextObject;
};

class PL_EDITORFRAMEWORK_DLL plGameObjectContextAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plGameObjectContextAction, plButtonAction);

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
