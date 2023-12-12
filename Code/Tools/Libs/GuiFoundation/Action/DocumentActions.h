#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

///
class PLASMA_GUIFOUNDATION_DLL plDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath, bool bForToolbar);
  static void MapToolsActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hSaveCategory;
  static plActionDescriptorHandle s_hSave;
  static plActionDescriptorHandle s_hSaveAs;
  static plActionDescriptorHandle s_hSaveAll;

  static plActionDescriptorHandle s_hCloseCategory;
  static plActionDescriptorHandle s_hClose;
  static plActionDescriptorHandle s_hOpenContainingFolder;
  static plActionDescriptorHandle s_hCopyAssetGuid;

  static plActionDescriptorHandle s_hUpdatePrefabs;
  static plActionDescriptorHandle s_hDocumentCategory;
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
