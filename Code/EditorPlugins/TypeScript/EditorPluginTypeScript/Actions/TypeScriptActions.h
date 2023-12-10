#pragma once

#include <EditorPluginTypeScript/EditorPluginTypeScriptDLL.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plTypeScriptAssetDocument;
struct plTypeScriptAssetEvent;

class plTypeScriptActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hEditScript;
};

class plTypeScriptAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTypeScriptAction, plButtonAction);

public:
  enum class ActionType
  {
    EditScript,
  };

  plTypeScriptAction(const plActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);

  virtual void Execute(const plVariant& value) override;

private:
  plTypeScriptAssetDocument* m_pDocument = nullptr;
  ActionType m_Type;
};
