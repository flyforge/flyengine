#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class PLASMA_EDITORFRAMEWORK_DLL plAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(const char* szMapping, const char* szPath);
  static void MapToolBarActions(const char* szMapping, bool bDocument);

  static plActionDescriptorHandle s_hAssetCategory;
  static plActionDescriptorHandle s_hTransformAsset;
  static plActionDescriptorHandle s_hTransformAllAssets;
  static plActionDescriptorHandle s_hResaveAllAssets;
  static plActionDescriptorHandle s_hCheckFileSystem;
  static plActionDescriptorHandle s_hWriteLookupTable;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plAssetAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAssetAction, plButtonAction);

public:
  enum class ButtonType
  {
    TransformAsset,
    TransformAllAssets,
    ResaveAllAssets,
    CheckFileSystem,
    WriteLookupTable,
  };

  plAssetAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plAssetAction();

  virtual void Execute(const plVariant& value) override;

private:
  ButtonType m_ButtonType;
};
