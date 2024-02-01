#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Document/Document.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCommandTransaction, 1, plRTTIDefaultAllocator<plCommandTransaction>)
PL_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// plCommandTransaction
////////////////////////////////////////////////////////////////////////

plCommandTransaction::plCommandTransaction()
{
  // doesn't do anything on its own
  m_bModifiedDocument = false;
}

plCommandTransaction::~plCommandTransaction()
{
  PL_ASSERT_DEV(m_ChildActions.IsEmpty(), "The list should be cleared in 'Cleanup'");
}

plStatus plCommandTransaction::DoInternal(bool bRedo)
{
  PL_ASSERT_DEV(bRedo == true, "Implementation error");
  return plStatus(PL_SUCCESS);
}

plStatus plCommandTransaction::UndoInternal(bool bFireEvents)
{
  return plStatus(PL_SUCCESS);
}

void plCommandTransaction::CleanupInternal(CommandState state) {}

plStatus plCommandTransaction::AddCommandTransaction(plCommand* pCommand)
{
  pCommand->m_pDocument = m_pDocument;
  m_ChildActions.PushBack(pCommand);
  return plStatus(PL_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// plCommandHistory
////////////////////////////////////////////////////////////////////////

plCommandHistory::plCommandHistory(plDocument* pDocument)
{
  auto pStorage = PL_DEFAULT_NEW(Storage);
  pStorage->m_pDocument = pDocument;
  SwapStorage(pStorage);

  m_bTemporaryMode = false;
  m_bIsInUndoRedo = false;
}

plCommandHistory::~plCommandHistory()
{
  if (m_pHistoryStorage->GetRefCount() == 1)
  {
    PL_ASSERT_ALWAYS(m_pHistoryStorage->m_UndoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
    PL_ASSERT_ALWAYS(m_pHistoryStorage->m_RedoHistory.IsEmpty(), "Must clear history before destructor as object manager will be dead already");
  }
}

void plCommandHistory::BeginTemporaryCommands(plStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands)
{
  PL_ASSERT_DEV(!m_bTemporaryMode, "Temporary Mode cannot be nested");
  StartTransaction(sDisplayString);
  StartTransaction("[Temporary]");

  m_bFireEventsWhenUndoingTempCommands = bFireEventsWhenUndoingTempCommands;
  m_bTemporaryMode = true;
  m_iTemporaryDepth = (plInt32)m_pHistoryStorage->m_TransactionStack.GetCount();
}

void plCommandHistory::CancelTemporaryCommands()
{
  EndTemporaryCommands(true);
  EndTransaction(true);
}

void plCommandHistory::FinishTemporaryCommands()
{
  EndTemporaryCommands(false);
  EndTransaction(false);
}

bool plCommandHistory::InTemporaryTransaction() const
{
  return m_bTemporaryMode;
}


void plCommandHistory::SuspendTemporaryTransaction()
{
  m_iPreSuspendTemporaryDepth = (plInt32)m_pHistoryStorage->m_TransactionStack.GetCount();
  PL_ASSERT_DEV(m_bTemporaryMode, "No temporary transaction active.");
  while (m_iTemporaryDepth < (plInt32)m_pHistoryStorage->m_TransactionStack.GetCount())
  {
    EndTransaction(true);
  }
  EndTemporaryCommands(true);
}

void plCommandHistory::ResumeTemporaryTransaction()
{
  PL_ASSERT_DEV(m_iTemporaryDepth == (plInt32)m_pHistoryStorage->m_TransactionStack.GetCount() + 1, "Can't resume temporary, not before temporary depth.");
  while (m_iPreSuspendTemporaryDepth > (plInt32)m_pHistoryStorage->m_TransactionStack.GetCount())
  {
    StartTransaction("[Temporary]");
  }
  m_bTemporaryMode = true;
  PL_ASSERT_DEV(m_iPreSuspendTemporaryDepth == (plInt32)m_pHistoryStorage->m_TransactionStack.GetCount(), "");
}

void plCommandHistory::EndTemporaryCommands(bool bCancel)
{
  PL_ASSERT_DEV(m_bTemporaryMode, "Temporary Mode was not enabled");
  PL_ASSERT_DEV(m_iTemporaryDepth == (plInt32)m_pHistoryStorage->m_TransactionStack.GetCount(), "Transaction stack is at depth {0} but temporary is at {1}",
    m_pHistoryStorage->m_TransactionStack.GetCount(), m_iTemporaryDepth);
  m_bTemporaryMode = false;

  EndTransaction(bCancel);
}

plStatus plCommandHistory::UndoInternal()
{
  PL_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  PL_ASSERT_DEV(m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Can't undo with active transaction!");
  PL_ASSERT_DEV(!m_pHistoryStorage->m_UndoHistory.IsEmpty(), "Can't undo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    plCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = plCommandHistoryEvent::Type::UndoStarted;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  plCommandTransaction* pTransaction = m_pHistoryStorage->m_UndoHistory.PeekBack();

  plStatus status = pTransaction->Undo(true);
  if (status.m_Result == PL_SUCCESS)
  {
    m_pHistoryStorage->m_UndoHistory.PopBack();
    m_pHistoryStorage->m_RedoHistory.PushBack(pTransaction);

    m_pHistoryStorage->m_pDocument->SetModified(true);

    status = plStatus(PL_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    plCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = plCommandHistoryEvent::Type::UndoEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
  return status;
}

plStatus plCommandHistory::Undo(plUInt32 uiNumEntries)
{
  for (plUInt32 i = 0; i < uiNumEntries; i++)
  {
    PL_SUCCEED_OR_RETURN(UndoInternal());
  }

  return plStatus(PL_SUCCESS);
}

plStatus plCommandHistory::RedoInternal()
{
  PL_ASSERT_DEV(!m_bIsInUndoRedo, "invalidly nested undo/redo");
  PL_ASSERT_DEV(m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Can't redo with active transaction!");
  PL_ASSERT_DEV(!m_pHistoryStorage->m_RedoHistory.IsEmpty(), "Can't redo with empty undo queue!");

  m_bIsInUndoRedo = true;
  {
    plCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = plCommandHistoryEvent::Type::RedoStarted;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  plCommandTransaction* pTransaction = m_pHistoryStorage->m_RedoHistory.PeekBack();

  plStatus status(PL_FAILURE);
  if (pTransaction->Do(true).m_Result == PL_SUCCESS)
  {
    m_pHistoryStorage->m_RedoHistory.PopBack();
    m_pHistoryStorage->m_UndoHistory.PushBack(pTransaction);

    m_pHistoryStorage->m_pDocument->SetModified(true);

    status = plStatus(PL_SUCCESS);
  }

  m_bIsInUndoRedo = false;
  {
    plCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = plCommandHistoryEvent::Type::RedoEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
  return status;
}

plStatus plCommandHistory::Redo(plUInt32 uiNumEntries)
{
  for (plUInt32 i = 0; i < uiNumEntries; i++)
  {
    PL_SUCCEED_OR_RETURN(RedoInternal());
  }

  return plStatus(PL_SUCCESS);
}

bool plCommandHistory::CanUndo() const
{
  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
    return false;

  return !m_pHistoryStorage->m_UndoHistory.IsEmpty();
}

bool plCommandHistory::CanRedo() const
{
  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
    return false;

  return !m_pHistoryStorage->m_RedoHistory.IsEmpty();
}


plStringView plCommandHistory::GetUndoDisplayString() const
{
  if (m_pHistoryStorage->m_UndoHistory.IsEmpty())
    return "";

  return m_pHistoryStorage->m_UndoHistory.PeekBack()->m_sDisplayString;
}


plStringView plCommandHistory::GetRedoDisplayString() const
{
  if (m_pHistoryStorage->m_RedoHistory.IsEmpty())
    return "";

  return m_pHistoryStorage->m_RedoHistory.PeekBack()->m_sDisplayString;
}

void plCommandHistory::StartTransaction(const plFormatString& displayString)
{
  PL_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot start new transaction while redoing/undoing.");

  /// \todo Allow to have a limited transaction history and clean up transactions after a while

  plCommandTransaction* pTransaction;

  if (m_bTemporaryMode && !m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    pTransaction = m_pHistoryStorage->m_TransactionStack.PeekBack();
    pTransaction->Undo(m_bFireEventsWhenUndoingTempCommands).IgnoreResult();
    pTransaction->Cleanup(plCommand::CommandState::WasUndone);
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
    return;
  }

  plStringBuilder tmp;

  pTransaction = plGetStaticRTTI<plCommandTransaction>()->GetAllocator()->Allocate<plCommandTransaction>();
  pTransaction->m_pDocument = m_pHistoryStorage->m_pDocument;
  pTransaction->m_sDisplayString = displayString.GetText(tmp);

  if (!m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    // Stacked transaction
    m_pHistoryStorage->m_TransactionStack.PeekBack()->AddCommandTransaction(pTransaction).AssertSuccess();
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
  }
  else
  {
    // Initial transaction
    m_pHistoryStorage->m_TransactionStack.PushBack(pTransaction);
    m_pHistoryStorage->m_ActiveCommandStack.PushBack(pTransaction);
    {
      plCommandHistoryEvent e;
      e.m_pDocument = m_pHistoryStorage->m_pDocument;
      e.m_Type = plCommandHistoryEvent::Type::TransactionStarted;
      m_pHistoryStorage->m_Events.Broadcast(e);
    }
  }
  return;
}

void plCommandHistory::EndTransaction(bool bCancel)
{
  PL_ASSERT_DEV(!m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Trying to end transaction without starting one!");

  if (m_pHistoryStorage->m_TransactionStack.GetCount() == 1)
  {
    /// Empty transactions are always canceled, so that they do not create an unnecessary undo action and clear the redo stack

    const bool bDidAnything = m_pHistoryStorage->m_TransactionStack.PeekBack()->HasChildActions();
    if (!bDidAnything)
      bCancel = true;

    plCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = bCancel ? plCommandHistoryEvent::Type::BeforeTransactionCanceled : plCommandHistoryEvent::Type::BeforeTransactionEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }

  if (!bCancel)
  {
    if (m_pHistoryStorage->m_TransactionStack.GetCount() > 1)
    {
      m_pHistoryStorage->m_TransactionStack.PopBack();
      m_pHistoryStorage->m_ActiveCommandStack.PopBack();
    }
    else
    {
      const bool bDidModifyDoc = m_pHistoryStorage->m_TransactionStack.PeekBack()->HasModifiedDocument();
      m_pHistoryStorage->m_UndoHistory.PushBack(m_pHistoryStorage->m_TransactionStack.PeekBack());
      m_pHistoryStorage->m_TransactionStack.PopBack();
      m_pHistoryStorage->m_ActiveCommandStack.PopBack();
      ClearRedoHistory();

      if (bDidModifyDoc)
      {
        m_pHistoryStorage->m_pDocument->SetModified(true);
      }
    }
  }
  else
  {
    plCommandTransaction* pTransaction = m_pHistoryStorage->m_TransactionStack.PeekBack();

    pTransaction->Undo(true).AssertSuccess();
    m_pHistoryStorage->m_TransactionStack.PopBack();
    m_pHistoryStorage->m_ActiveCommandStack.PopBack();

    if (m_pHistoryStorage->m_TransactionStack.IsEmpty())
    {
      pTransaction->Cleanup(plCommand::CommandState::WasUndone);
      pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);
    }
  }

  if (m_pHistoryStorage->m_TransactionStack.IsEmpty())
  {
    // All transactions done
    plCommandHistoryEvent e;
    e.m_pDocument = m_pHistoryStorage->m_pDocument;
    e.m_Type = bCancel ? plCommandHistoryEvent::Type::TransactionCanceled : plCommandHistoryEvent::Type::TransactionEnded;
    m_pHistoryStorage->m_Events.Broadcast(e);
  }
}

plStatus plCommandHistory::AddCommand(plCommand& ref_command)
{
  PL_ASSERT_DEV(!m_pHistoryStorage->m_TransactionStack.IsEmpty(), "Cannot add command while no transaction is started");
  PL_ASSERT_DEV(!m_pHistoryStorage->m_ActiveCommandStack.IsEmpty(), "Transaction stack is not synced anymore with m_ActiveCommandStack");

  auto res = m_pHistoryStorage->m_ActiveCommandStack.PeekBack()->AddSubCommand(ref_command);

  // Error handling should be on the caller side.
  // if (res.Failed() && !res.m_sMessage.IsEmpty())
  //{
  //  plLog::Error("Command failed: '{0}'", res.m_sMessage);
  //}

  return res;
}

void plCommandHistory::ClearUndoHistory()
{
  PL_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_pHistoryStorage->m_UndoHistory.IsEmpty())
  {
    plCommandTransaction* pTransaction = m_pHistoryStorage->m_UndoHistory.PeekBack();

    pTransaction->Cleanup(plCommand::CommandState::WasDone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_pHistoryStorage->m_UndoHistory.PopBack();
  }
}

void plCommandHistory::ClearRedoHistory()
{
  PL_ASSERT_DEV(!m_bIsInUndoRedo, "Cannot clear undo/redo history while redoing/undoing.");
  while (!m_pHistoryStorage->m_RedoHistory.IsEmpty())
  {
    plCommandTransaction* pTransaction = m_pHistoryStorage->m_RedoHistory.PeekBack();

    pTransaction->Cleanup(plCommand::CommandState::WasUndone);
    pTransaction->GetDynamicRTTI()->GetAllocator()->Deallocate(pTransaction);

    m_pHistoryStorage->m_RedoHistory.PopBack();
  }
}

void plCommandHistory::MergeLastTwoTransactions()
{
  /// \todo This would not be necessary, if hierarchical transactions would not crash

  PL_ASSERT_DEV(m_pHistoryStorage->m_RedoHistory.IsEmpty(), "This can only be called directly after EndTransaction, when the redo history is empty");
  PL_ASSERT_DEV(m_pHistoryStorage->m_UndoHistory.GetCount() >= 2, "Can only do this when at least two transcations are in the queue");

  plCommandTransaction* pLast = m_pHistoryStorage->m_UndoHistory.PeekBack();
  m_pHistoryStorage->m_UndoHistory.PopBack();

  plCommandTransaction* pNowLast = m_pHistoryStorage->m_UndoHistory.PeekBack();
  pNowLast->m_ChildActions.PushBackRange(pLast->m_ChildActions);

  pLast->m_ChildActions.Clear();

  pLast->GetDynamicRTTI()->GetAllocator()->Deallocate(pLast);
}

plUInt32 plCommandHistory::GetUndoStackSize() const
{
  return m_pHistoryStorage->m_UndoHistory.GetCount();
}

plUInt32 plCommandHistory::GetRedoStackSize() const
{
  return m_pHistoryStorage->m_RedoHistory.GetCount();
}

const plCommandTransaction* plCommandHistory::GetUndoStackEntry(plUInt32 uiIndex) const
{
  return m_pHistoryStorage->m_UndoHistory[GetUndoStackSize() - 1 - uiIndex];
}

const plCommandTransaction* plCommandHistory::GetRedoStackEntry(plUInt32 uiIndex) const
{
  return m_pHistoryStorage->m_RedoHistory[GetRedoStackSize() - 1 - uiIndex];
}

plSharedPtr<plCommandHistory::Storage> plCommandHistory::SwapStorage(plSharedPtr<plCommandHistory::Storage> pNewStorage)
{
  PL_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  PL_ASSERT_DEV(!m_bIsInUndoRedo, "Can't be in Undo/Redo when swapping storage.");

  auto retVal = m_pHistoryStorage;

  m_EventsUnsubscriber.Unsubscribe();

  m_pHistoryStorage = pNewStorage;

  m_pHistoryStorage->m_Events.AddEventHandler([this](const plCommandHistoryEvent& e) { m_Events.Broadcast(e); },
    m_EventsUnsubscriber);

  return retVal;
}
