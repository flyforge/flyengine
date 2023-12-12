#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

///
class PLASMA_GUIFOUNDATION_DLL plCommandHistoryActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hCommandHistoryCategory;
  static plActionDescriptorHandle s_hUndo;
  static plActionDescriptorHandle s_hRedo;
};


///
class PLASMA_GUIFOUNDATION_DLL plCommandHistoryAction : public plDynamicActionAndMenuAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCommandHistoryAction, plDynamicActionAndMenuAction);

public:
  enum class ButtonType
  {
    Undo,
    Redo,
  };

  plCommandHistoryAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plCommandHistoryAction();

  virtual void Execute(const plVariant& value) override;
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_Entries) override;

private:
  void UpdateState();
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);

  ButtonType m_ButtonType;
};
