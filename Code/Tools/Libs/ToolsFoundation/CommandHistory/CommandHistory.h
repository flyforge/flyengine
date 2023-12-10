#pragma once

#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plCommandHistory;

class PLASMA_TOOLSFOUNDATION_DLL plCommandTransaction : public plCommand
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCommandTransaction, plCommand);

public:
  plCommandTransaction();
  ~plCommandTransaction();

  plString m_sDisplayString;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;
  plStatus AddCommandTransaction(plCommand* command);

private:
  friend class plCommandHistory;
};

struct plCommandHistoryEvent
{
  enum class Type
  {
    UndoStarted,
    UndoEnded,
    RedoStarted,
    RedoEnded,
    TransactionStarted,        ///< Emit after initial transaction started.
    BeforeTransactionEnded,    ///< Emit before initial transaction ended.
    BeforeTransactionCanceled, ///< Emit before initial transaction ended.
    TransactionEnded,          ///< Emit after initial transaction ended.
    TransactionCanceled,       ///< Emit after initial transaction canceled.
    HistoryChanged,
  };

  Type m_Type;
  const plDocument* m_pDocument;
};

/// \brief Stores the undo / redo stacks of transactions done on a document.
class PLASMA_TOOLSFOUNDATION_DLL plCommandHistory
{
public:
  plEvent<const plCommandHistoryEvent&, plMutex> m_Events;

  // \brief Storage for the command history so it can be swapped when using multiple sub documents.
  class Storage : public plRefCounted
  {
  public:
    plHybridArray<plCommandTransaction*, 4> m_TransactionStack;
    plHybridArray<plCommand*, 4> m_ActiveCommandStack;
    plDeque<plCommandTransaction*> m_UndoHistory;
    plDeque<plCommandTransaction*> m_RedoHistory;
    plDocument* m_pDocument = nullptr;
    plEvent<const plCommandHistoryEvent&, plMutex> m_Events;
  };

public:
  plCommandHistory(plDocument* pDocument);
  ~plCommandHistory();

  const plDocument* GetDocument() const { return m_pHistoryStorage->m_pDocument; }

  plStatus Undo(plUInt32 uiNumEntries = 1);
  plStatus Redo(plUInt32 uiNumEntries = 1);

  bool CanUndo() const;
  bool CanRedo() const;

  plStringView GetUndoDisplayString() const;
  plStringView GetRedoDisplayString() const;

  void StartTransaction(const plFormatString& displayString);
  void CancelTransaction() { EndTransaction(true); }
  void FinishTransaction() { EndTransaction(false); }

  /// \brief Returns true, if between StartTransaction / EndTransaction. False during Undo/Redo.
  bool IsInTransaction() const { return !m_pHistoryStorage->m_TransactionStack.IsEmpty(); }
  bool IsInUndoRedo() const { return m_bIsInUndoRedo; }

  /// \brief Call this to start a series of transactions that typically change the same value over and over (e.g. dragging an object to a position).
  /// Every time a new transaction is started, the previous one is undone first. At the end of a series of temporary transactions, only the last
  /// transaction will be stored as a single undo step. Call this first and then start a transaction inside it.
  void BeginTemporaryCommands(plStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  void CancelTemporaryCommands();
  void FinishTemporaryCommands();

  bool InTemporaryTransaction() const;
  void SuspendTemporaryTransaction();
  void ResumeTemporaryTransaction();

  plStatus AddCommand(plCommand& ref_command);

  void ClearUndoHistory();
  void ClearRedoHistory();

  void MergeLastTwoTransactions();

  plUInt32 GetUndoStackSize() const;
  plUInt32 GetRedoStackSize() const;
  const plCommandTransaction* GetUndoStackEntry(plUInt32 uiIndex) const;
  const plCommandTransaction* GetRedoStackEntry(plUInt32 uiIndex) const;

  plSharedPtr<plCommandHistory::Storage> SwapStorage(plSharedPtr<plCommandHistory::Storage> pNewStorage);
  plSharedPtr<plCommandHistory::Storage> GetStorage() { return m_pHistoryStorage; }

private:
  friend class plCommand;

  plStatus UndoInternal();
  plStatus RedoInternal();

  void EndTransaction(bool bCancel);
  void EndTemporaryCommands(bool bCancel);

  plSharedPtr<plCommandHistory::Storage> m_pHistoryStorage;

  plEvent<const plCommandHistoryEvent&, plMutex>::Unsubscriber m_EventsUnsubscriber;

  bool m_bFireEventsWhenUndoingTempCommands = false;
  bool m_bTemporaryMode = false;
  plInt32 m_iTemporaryDepth = -1;
  plInt32 m_iPreSuspendTemporaryDepth = -1;
  bool m_bIsInUndoRedo = false;
};
