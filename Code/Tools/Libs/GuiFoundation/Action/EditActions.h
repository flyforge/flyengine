#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Selection/SelectionManager.h>
///
class PLASMA_GUIFOUNDATION_DLL plEditActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath, bool bDeleteAction, bool bAdvancedPasteActions);
  static void MapContextMenuActions(const char* szMapping, const char* szPath);
  static void MapViewContextMenuActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hEditCategory;
  static plActionDescriptorHandle s_hCopy;
  static plActionDescriptorHandle s_hPaste;
  static plActionDescriptorHandle s_hPasteAsChild;
  static plActionDescriptorHandle s_hPasteAtOriginalLocation;
  static plActionDescriptorHandle s_hDelete;
};


///
class PLASMA_GUIFOUNDATION_DLL plEditAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plEditAction, plButtonAction);

public:
  enum class ButtonType
  {
    Copy,
    Paste,
    PasteAsChild,
    PasteAtOriginalLocation,
    Delete,
  };
  plEditAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plEditAction();

  virtual void Execute(const plVariant& value) override;

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  ButtonType m_ButtonType;
};
