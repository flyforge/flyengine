#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QKeySequence>
#include <ToolsFoundation/Document/DocumentManager.h>

class QWidget;
struct plActionDescriptor;
class plAction;
struct plActionContext;

using plActionId = plGenericId<24, 8>;
using CreateActionFunc = plAction* (*)(const plActionContext&);
using DeleteActionFunc = void (*)(plAction*);

/// \brief Handle for a plAction.
///
/// plAction can be invalidated at runtime so don't store them.
class PLASMA_GUIFOUNDATION_DLL plActionDescriptorHandle
{
public:
  using StorageType = plUInt32;

  PLASMA_DECLARE_HANDLE_TYPE(plActionDescriptorHandle, plActionId);
  friend class plActionManager;

public:
  const plActionDescriptor* GetDescriptor() const;
};

///
struct plActionScope
{
  enum Enum
  {
    Global,
    Document,
    Window,
    Default = Global
  };
  using StorageType = plUInt8;
};

///
struct plActionType
{
  enum Enum
  {
    Action,
    Category,
    Menu,
    ActionAndMenu,
    Default = Action
  };
  using StorageType = plUInt8;
};

///
struct PLASMA_GUIFOUNDATION_DLL plActionContext
{
  plActionContext() = default;
  plActionContext(plDocument* pDoc) { m_pDocument = pDoc; }

  plDocument* m_pDocument = nullptr;
  plString m_sMapping;
  QWidget* m_pWindow = nullptr;
};


///
struct PLASMA_GUIFOUNDATION_DLL plActionDescriptor
{
  plActionDescriptor() = default;
  ;
  plActionDescriptor(plActionType::Enum type, plActionScope::Enum scope, const char* szName, const char* szCategoryPath, const char* szShortcut,
    CreateActionFunc createAction, DeleteActionFunc deleteAction = nullptr);

  plActionDescriptorHandle m_Handle;
  plEnum<plActionType> m_Type;

  plEnum<plActionScope> m_Scope;
  plString m_sActionName;   ///< Unique within category path, shown in key configuration dialog
  plString m_sCategoryPath; ///< Category in key configuration dialog, e.g. "Tree View" or "File"

  plString m_sShortcut;
  plString m_sDefaultShortcut;

  plAction* CreateAction(const plActionContext& context) const;
  void DeleteAction(plAction* pAction) const;

  void UpdateExistingActions();

private:
  CreateActionFunc m_CreateAction;
  DeleteActionFunc m_DeleteAction;

  mutable plHybridArray<plAction*, 4> m_CreatedActions;
};



///
class PLASMA_GUIFOUNDATION_DLL plAction : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAction, plReflectedClass);
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plAction);

public:
  plAction(const plActionContext& context) { m_Context = context; }
  virtual void Execute(const plVariant& value) = 0;

  void TriggerUpdate();
  const plActionContext& GetContext() const { return m_Context; }
  plActionDescriptorHandle GetDescriptorHandle() { return m_hDescriptorHandle; }

public:
  plEvent<plAction*> m_StatusUpdateEvent; ///< Fire when the state of the action changes (enabled, value etc...)

protected:
  plActionContext m_Context;

private:
  friend struct plActionDescriptor;
  plActionDescriptorHandle m_hDescriptorHandle;
};
