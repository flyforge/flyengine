#pragma once

#include <EditorPluginJolt/EditorPluginJoltDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class PLASMA_EDITORPLUGINJOLT_DLL plJoltActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static plActionDescriptorHandle s_hCategoryJolt;
  static plActionDescriptorHandle s_hProjectSettings;
};

class PLASMA_EDITORPLUGINJOLT_DLL plJoltAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltAction, plButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
  };

  plJoltAction(const plActionContext& context, const char* szName, ActionType type);
  ~plJoltAction();

  virtual void Execute(const plVariant& value) override;

private:
  ActionType m_Type;
};
