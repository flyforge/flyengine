#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plSkeletonAssetDocument;
struct plSkeletonAssetEvent;

class plSkeletonActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hRenderBones;
  static plActionDescriptorHandle s_hRenderColliders;
  static plActionDescriptorHandle s_hRenderJoints;
  static plActionDescriptorHandle s_hRenderSwingLimits;
  static plActionDescriptorHandle s_hRenderTwistLimits;
  static plActionDescriptorHandle s_hRenderPreviewMesh;
};

class plSkeletonAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSkeletonAction, plButtonAction);

public:
  enum class ActionType
  {
    RenderBones,
    RenderColliders,
    RenderJoints,
    RenderSwingLimits,
    RenderTwistLimits,
    RenderPreviewMesh,
  };

  plSkeletonAction(const plActionContext& context, const char* szName, ActionType type);
  ~plSkeletonAction();

  virtual void Execute(const plVariant& value) override;

private:
  void AssetEventHandler(const plSkeletonAssetEvent& e);
  void UpdateState();

  plSkeletonAssetDocument* m_pSkeletonpDocument = nullptr;
  ActionType m_Type;
};
