#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

class plVisualShaderActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping);

  static plActionDescriptorHandle s_hCleanGraph;
};

class plVisualShaderAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisualShaderAction, plButtonAction);

public:
  plVisualShaderAction(const plActionContext& context, const char* name);
  ~plVisualShaderAction();

  virtual void Execute(const plVariant& value) override;
};
