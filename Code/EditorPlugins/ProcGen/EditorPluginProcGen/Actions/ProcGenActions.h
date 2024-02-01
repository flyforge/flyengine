#pragma once

#include <EditorPluginProcGen/EditorPluginProcGenDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plPreferences;

class PL_EDITORPLUGINPROCGEN_DLL plProcGenActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions();

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hDumpAST;
  static plActionDescriptorHandle s_hDumpDisassembly;
};

class PL_EDITORPLUGINPROCGEN_DLL plProcGenAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plProcGenAction, plButtonAction);

public:
  enum class ActionType
  {
    DumpAST,
    DumpDisassembly,
  };

  plProcGenAction(const plActionContext& context, const char* szName, ActionType type);
  ~plProcGenAction();

  virtual void Execute(const plVariant& value) override;

private:
  void OnPreferenceChange(plPreferences* pref);

  ActionType m_Type;
};
