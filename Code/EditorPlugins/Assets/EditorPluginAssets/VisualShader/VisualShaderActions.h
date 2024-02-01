#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class plVisualShaderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);

  static plActionDescriptorHandle s_hCleanGraph;
};

class plVisualShaderAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plVisualShaderAction, plButtonAction);

public:
  plVisualShaderAction(const plActionContext& context, const char* szName);
  ~plVisualShaderAction();

  virtual void Execute(const plVariant& value) override;
};
