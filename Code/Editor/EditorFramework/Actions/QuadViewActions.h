#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class PL_EDITORFRAMEWORK_DLL plQuadViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hToggleViews;
  static plActionDescriptorHandle s_hSpawnView;
};

///
class PL_EDITORFRAMEWORK_DLL plQuadViewAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plQuadViewAction, plButtonAction);

public:
  enum class ButtonType
  {
    ToggleViews,
    SpawnView,
  };

  plQuadViewAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plQuadViewAction();

  virtual void Execute(const plVariant& value) override;

private:
  ButtonType m_ButtonType;
};
