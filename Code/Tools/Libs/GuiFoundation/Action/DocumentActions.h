#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class PLASMA_GUIFOUNDATION_DLL plDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(plStringView sMapping, plStringView sTargetMenu = "G.File.Common");
  static void MapToolbarActions(plStringView sMapping);
  static void MapToolsActions(plStringView sMapping);

  static plActionDescriptorHandle s_hSaveCategory;
  static plActionDescriptorHandle s_hSave;
  static plActionDescriptorHandle s_hSaveAs;
  static plActionDescriptorHandle s_hSaveAll;

  static plActionDescriptorHandle s_hClose;
  static plActionDescriptorHandle s_hCloseAll;
  static plActionDescriptorHandle s_hCloseAllButThis;

  static plActionDescriptorHandle s_hOpenContainingFolder;
  static plActionDescriptorHandle s_hCopyAssetGuid;

  static plActionDescriptorHandle s_hUpdatePrefabs;
};


/// \brief Standard document actions.
class PLASMA_GUIFOUNDATION_DLL plDocumentAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDocumentAction, plButtonAction);

public:
  enum class ButtonType
  {
    Save,
    SaveAs,
    SaveAll,
    Close,
    CloseAll,
    CloseAllButThis,
    OpenContainingFolder,
    UpdatePrefabs,
    CopyAssetGuid,
  };
  plDocumentAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plDocumentAction();

  virtual void Execute(const plVariant& value) override;

private:
  void DocumentEventHandler(const plDocumentEvent& e);

  ButtonType m_ButtonType;
};
