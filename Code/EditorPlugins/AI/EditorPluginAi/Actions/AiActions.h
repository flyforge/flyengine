#pragma once

#include <EditorPluginAi/EditorPluginAiDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class PL_EDITORPLUGINAI_DLL plAiActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static plActionDescriptorHandle s_hCategoryAi;
  static plActionDescriptorHandle s_hProjectSettings;
};

class PL_EDITORPLUGINAI_DLL plAiAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plAiAction, plButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
  };

  plAiAction(const plActionContext& context, const char* szName, ActionType type);
  ~plAiAction();

  virtual void Execute(const plVariant& value) override;

private:
  ActionType m_Type;
};
