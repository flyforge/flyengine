#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plDocument;
class plCommandTransaction;

/// \brief Interface for a command
///
/// Commands are the only objects that have non-const access to any data structures (contexts, documents etc.).
/// Thus, any modification must go through a command and the plCommandHistory is the only class capable of executing commands.
class PLASMA_TOOLSFOUNDATION_DLL plCommand : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCommand, plReflectedClass);

public:
  plCommand();
  ~plCommand();

  bool IsUndoable() const { return m_bUndoable; };
  bool HasChildActions() const { return !m_ChildActions.IsEmpty(); }
  bool HasModifiedDocument() const;

  enum class CommandState
  {
    WasDone,
    WasUndone
  };

protected:
  plStatus Do(bool bRedo);
  plStatus Undo(bool bFireEvents);
  void Cleanup(CommandState state);

  plStatus AddSubCommand(plCommand& command);
  plDocument* GetDocument() { return m_pDocument; };

private:
  virtual bool HasReturnValues() const { return false; }
  virtual plStatus DoInternal(bool bRedo) = 0;
  virtual plStatus UndoInternal(bool bFireEvents) = 0;
  virtual void CleanupInternal(CommandState state) = 0;

protected:
  friend class plCommandHistory;
  friend class plCommandTransaction;

  plString m_sDescription;
  bool m_bUndoable = true;
  bool m_bModifiedDocument = true;
  plHybridArray<plCommand*, 8> m_ChildActions;
  plDocument* m_pDocument = nullptr;
};
