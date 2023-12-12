#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class PLASMA_EDITORFRAMEWORK_DLL plQuadViewActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hToggleViews;
  static plActionDescriptorHandle s_hSpawnView;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plQuadViewAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plQuadViewAction, plButtonAction);

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
