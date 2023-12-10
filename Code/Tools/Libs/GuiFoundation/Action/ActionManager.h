#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>

/// \brief Registers an plAction whose constructor takes no arguments.
#define PLASMA_REGISTER_ACTION_0(ActionName, Scope, CategoryName, ShortCut, ActionClass)                                  \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(ActionClass, context, ActionName); }));

/// \brief Registers an plAction whose constructor takes one argument.
#define PLASMA_REGISTER_ACTION_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                          \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

/// \brief Registers an plAction whose constructor takes two arguments.
#define PLASMA_REGISTER_ACTION_2(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1, Param2)                  \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::Action, Scope, ActionName, CategoryName, ShortCut, \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(ActionClass, context, ActionName, Param1, Param2); }));

/// \brief Registers an plDynamicMenuAction
#define PLASMA_REGISTER_DYNAMIC_MENU(ActionName, ActionClass, IconPath)                                                  \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::Menu, plActionScope::Default, ActionName, "", "", \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(ActionClass, context, ActionName, IconPath); }));

/// \brief Registers an plDynamicActionAndMenuAction.
#define PLASMA_REGISTER_ACTION_AND_DYNAMIC_MENU_1(ActionName, Scope, CategoryName, ShortCut, ActionClass, Param1)                \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::ActionAndMenu, Scope, ActionName, CategoryName, ShortCut, \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(ActionClass, context, ActionName, Param1); }));

/// \brief Registers a category that should be treated as a sub-menu.
#define PLASMA_REGISTER_MENU(ActionName)                                                                                 \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::Menu, plActionScope::Default, ActionName, "", "", \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(plMenuAction, context, ActionName, ""); }));

/// \brief Registers a category that should be treated as a sub-menu and specifies a custom QIcon path.
#define PLASMA_REGISTER_MENU_WITH_ICON(ActionName, IconPath)                                                             \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::Menu, plActionScope::Default, ActionName, "", "", \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(plMenuAction, context, ActionName, IconPath); }));

/// \brief Registers a category that should just be a grouped area in a menu, but no dedicated sub-menu.
#define PLASMA_REGISTER_CATEGORY(CategoryName)                                                                                 \
  plActionManager::RegisterAction(plActionDescriptor(plActionType::Category, plActionScope::Default, CategoryName, "", "", \
    [](const plActionContext& context) -> plAction* { return PLASMA_DEFAULT_NEW(plCategoryAction, context); }));

/// \brief Stores 'actions' (things that can be triggered from UI).
///
/// Actions are usually represented by a button in a toolbar, or a menu entry.
/// Actions are unique across the entire application. Each action is registered exactly once,
/// but it may be referenced by many different plActionMap instances, which defines how an action shows up in a window.
///
/// Through RegisterAction() / UnregisterAction() an action is added or removed.
/// These functions are usually not called directly, but rather the macros at the top of this file are used (see PLASMA_REGISTER_CATEGORY, PLASMA_REGISTER_MENU, PLASMA_REGISTER_ACTION_X, ...).
///
/// Unit tests can call ExecuteAction() to directly invoke an action.
/// Widgets use plActionMap to organize which actions are available in a window, and how they are structured.
/// For instance, the same action can appear in a menu, in a toolbar and a context menu. In each case their location may be different (top-level, in a sub-menu, etc).
/// See plActionMap for details.
class PLASMA_GUIFOUNDATION_DLL plActionManager
{
public:
  static plActionDescriptorHandle RegisterAction(const plActionDescriptor& desc);
  static bool UnregisterAction(plActionDescriptorHandle& ref_hAction);
  static const plActionDescriptor* GetActionDescriptor(plActionDescriptorHandle hAction);
  static plActionDescriptorHandle GetActionHandle(const char* szCategory, const char* szActionName);

  /// \brief Searches all action categories for the given action name. Returns the category name in which the action name was found, or an empty
  /// string.
  static plString FindActionCategory(const char* szActionName);

  /// \brief Quick way to execute an action from code
  ///
  /// The use case is mostly for unit tests, which need to execute actions directly and without a link dependency on
  /// the code that registered the action.
  ///
  /// \param szCategory The category of the action, ie. under which name the action appears in the Shortcut binding dialog.
  ///        For example "Scene", "Scene - Cameras", "Scene - Selection", "Assets" etc.
  ///        This parameter may be nullptr in which case FindActionCategory(szActionName) is used to try to detect the category automatically.
  /// \param szActionName The name (not mapped path) under which the action was registered.
  ///        For example "Selection.Copy", "Prefabs.ConvertToEngine", "Scene.Camera.SnapObjectToCamera"
  /// \param context The context in which to execute the action. Depending on the plActionScope of the target action,
  ///        some members are optional. E.g. for document actions, only the m_pDocument member must be specified.
  /// \param value Optional value passed through to the plAction::Execute() call. Some actions use it, most don't.
  /// \return Returns failure in case the action could not be found.
  static plResult ExecuteAction(const char* szCategory, const char* szActionName, const plActionContext& context, const plVariant& value = plVariant());

  static void SaveShortcutAssignment();
  static void LoadShortcutAssignment();

  static const plIdTable<plActionId, plActionDescriptor*>::ConstIterator GetActionIterator();

  struct Event
  {
    enum class Type
    {
      ActionAdded,
      ActionRemoved
    };

    Type m_Type;
    const plActionDescriptor* m_pDesc;
    plActionDescriptorHandle m_Handle;
  };

  static plEvent<const Event&> s_Events;

private:
  PLASMA_MAKE_SUBSYSTEM_STARTUP_FRIEND(GuiFoundation, ActionManager);

  static void Startup();
  static void Shutdown();
  static plActionDescriptor* CreateActionDesc(const plActionDescriptor& desc);
  static void DeleteActionDesc(plActionDescriptor* pDesc);

  struct CategoryData
  {
    plSet<plActionDescriptorHandle> m_Actions;
    plHashTable<const char*, plActionDescriptorHandle> m_ActionNameToHandle;
  };

private:
  static plIdTable<plActionId, plActionDescriptor*> s_ActionTable;
  static plMap<plString, CategoryData> s_CategoryPathToActions;
  static plMap<plString, plString> s_ShortcutOverride;
};
