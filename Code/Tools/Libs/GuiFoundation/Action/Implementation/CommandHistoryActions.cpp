#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCommandHistoryAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plCommandHistoryActions::s_hCommandHistoryCategory;
plActionDescriptorHandle plCommandHistoryActions::s_hUndo;
plActionDescriptorHandle plCommandHistoryActions::s_hRedo;

void plCommandHistoryActions::RegisterActions()
{
  s_hCommandHistoryCategory = PLASMA_REGISTER_CATEGORY("CmdHistoryCategory");
  s_hUndo = PLASMA_REGISTER_ACTION_AND_DYNAMIC_MENU_1("Document.Undo", plActionScope::Document, "Document", "Ctrl+Z", plCommandHistoryAction, plCommandHistoryAction::ButtonType::Undo);
  s_hRedo = PLASMA_REGISTER_ACTION_AND_DYNAMIC_MENU_1("Document.Redo", plActionScope::Document, "Document", "Ctrl+Y", plCommandHistoryAction, plCommandHistoryAction::ButtonType::Redo);
}

void plCommandHistoryActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCommandHistoryCategory);
  plActionManager::UnregisterAction(s_hUndo);
  plActionManager::UnregisterAction(s_hRedo);
}

void plCommandHistoryActions::MapActions(plStringView sMapping, plStringView sTargetMenu)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCommandHistoryCategory, sTargetMenu, 3.0f);
  pMap->MapAction(s_hUndo, sTargetMenu, "CmdHistoryCategory", 1.0f);
  pMap->MapAction(s_hRedo, sTargetMenu, "CmdHistoryCategory", 2.0f);
}

plCommandHistoryAction::plCommandHistoryAction(const plActionContext& context, const char* szName, ButtonType button)
  : plDynamicActionAndMenuAction(context, szName, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case plCommandHistoryAction::ButtonType::Undo:
      SetIconPath(":/GuiFoundation/Icons/Undo.svg");
      break;
    case plCommandHistoryAction::ButtonType::Redo:
      SetIconPath(":/GuiFoundation/Icons/Redo.svg");
      break;
  }

  m_Context.m_pDocument->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plCommandHistoryAction::CommandHistoryEventHandler, this));

  UpdateState();
}

plCommandHistoryAction::~plCommandHistoryAction()
{
  m_Context.m_pDocument->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plCommandHistoryAction::CommandHistoryEventHandler, this));
}

void plCommandHistoryAction::GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries)
{
  out_entries.Clear();

  plCommandHistory* pHistory = m_Context.m_pDocument->GetCommandHistory();

  const plUInt32 iCount = (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackSize() : pHistory->GetRedoStackSize();
  for (plUInt32 i = 0; i < iCount; i++)
  {
    const plCommandTransaction* pTransaction = (m_ButtonType == ButtonType::Undo) ? pHistory->GetUndoStackEntry(i) : pHistory->GetRedoStackEntry(i);
    plDynamicMenuAction::Item entryItem;
    entryItem.m_sDisplay = pTransaction->m_sDisplayString;
    entryItem.m_UserValue = (plUInt32)i + 1; // Number of steps to undo / redo.
    out_entries.PushBack(entryItem);
  }
}

void plCommandHistoryAction::Execute(const plVariant& value)
{
  plUInt32 iCount = value.IsValid() ? value.ConvertTo<plUInt32>() : 1;

  switch (m_ButtonType)
  {
    case ButtonType::Undo:
    {
      PLASMA_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanUndo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Undo(iCount);
      plQtUiServices::MessageBoxStatus(stat, "Could not execute the Undo operation");
    }
    break;

    case ButtonType::Redo:
    {
      PLASMA_ASSERT_DEV(m_Context.m_pDocument->GetCommandHistory()->CanRedo(), "The action should not be active");

      auto stat = m_Context.m_pDocument->GetCommandHistory()->Redo(iCount);
      plQtUiServices::MessageBoxStatus(stat, "Could not execute the Redo operation");
    }
    break;
  }
}

void plCommandHistoryAction::UpdateState()
{
  switch (m_ButtonType)
  {
    case ButtonType::Undo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetUndoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanUndo());
      break;

    case ButtonType::Redo:
      SetAdditionalDisplayString(m_Context.m_pDocument->GetCommandHistory()->GetRedoDisplayString(), false);
      SetEnabled(m_Context.m_pDocument->GetCommandHistory()->CanRedo());
      break;
  }
}

void plCommandHistoryAction::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
{
  UpdateState();
}
