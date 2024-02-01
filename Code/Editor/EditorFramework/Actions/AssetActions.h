#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

///
class PL_EDITORFRAMEWORK_DLL plAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(plStringView sMapping);
  static void MapToolBarActions(plStringView sMapping, bool bDocument);

  static plActionDescriptorHandle s_hAssetCategory;
  static plActionDescriptorHandle s_hTransformAsset;
  static plActionDescriptorHandle s_hTransformAllAssets;
  static plActionDescriptorHandle s_hResaveAllAssets;
  static plActionDescriptorHandle s_hCheckFileSystem;
  static plActionDescriptorHandle s_hWriteLookupTable;
  static plActionDescriptorHandle s_hWriteDependencyDGML;
};

///
class PL_EDITORFRAMEWORK_DLL plAssetAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plAssetAction, plButtonAction);

public:
  enum class ButtonType
  {
    TransformAsset,
    TransformAllAssets,
    ResaveAllAssets,
    CheckFileSystem,
    WriteLookupTable,
    WriteDependencyDGML,
  };

  plAssetAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plAssetAction();

  virtual void Execute(const plVariant& value) override;

private:
  ButtonType m_ButtonType;
};
