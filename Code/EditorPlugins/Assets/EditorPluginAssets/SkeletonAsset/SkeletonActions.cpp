#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonActions.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkeletonAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plActionDescriptorHandle plSkeletonActions::s_hCategory;
plActionDescriptorHandle plSkeletonActions::s_hRenderBones;
plActionDescriptorHandle plSkeletonActions::s_hRenderColliders;
plActionDescriptorHandle plSkeletonActions::s_hRenderJoints;
plActionDescriptorHandle plSkeletonActions::s_hRenderSwingLimits;
plActionDescriptorHandle plSkeletonActions::s_hRenderTwistLimits;
plActionDescriptorHandle plSkeletonActions::s_hRenderPreviewMesh;

void plSkeletonActions::RegisterActions()
{
  s_hCategory = PLASMA_REGISTER_CATEGORY("SkeletonCategory");
  s_hRenderBones = PLASMA_REGISTER_ACTION_1("Skeleton.RenderBones", plActionScope::Document, "Skeletons", "", plSkeletonAction, plSkeletonAction::ActionType::RenderBones);
  s_hRenderColliders = PLASMA_REGISTER_ACTION_1("Skeleton.RenderColliders", plActionScope::Document, "Skeletons", "", plSkeletonAction, plSkeletonAction::ActionType::RenderColliders);
  s_hRenderJoints = PLASMA_REGISTER_ACTION_1("Skeleton.RenderJoints", plActionScope::Document, "Skeletons", "", plSkeletonAction, plSkeletonAction::ActionType::RenderJoints);
  s_hRenderSwingLimits = PLASMA_REGISTER_ACTION_1("Skeleton.RenderSwingLimits", plActionScope::Document, "Skeletons", "", plSkeletonAction, plSkeletonAction::ActionType::RenderSwingLimits);
  s_hRenderTwistLimits = PLASMA_REGISTER_ACTION_1("Skeleton.RenderTwistLimits", plActionScope::Document, "Skeletons", "", plSkeletonAction, plSkeletonAction::ActionType::RenderTwistLimits);
  s_hRenderPreviewMesh = PLASMA_REGISTER_ACTION_1("Skeleton.RenderPreviewMesh", plActionScope::Document, "Skeletons", "", plSkeletonAction, plSkeletonAction::ActionType::RenderPreviewMesh);
}

void plSkeletonActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCategory);
  plActionManager::UnregisterAction(s_hRenderBones);
  plActionManager::UnregisterAction(s_hRenderColliders);
  plActionManager::UnregisterAction(s_hRenderJoints);
  plActionManager::UnregisterAction(s_hRenderSwingLimits);
  plActionManager::UnregisterAction(s_hRenderTwistLimits);
  plActionManager::UnregisterAction(s_hRenderPreviewMesh);
}

void plSkeletonActions::MapActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hCategory, "", 11.0f);

  const char* szSubPath = "SkeletonCategory";

  pMap->MapAction(s_hRenderBones, szSubPath, 1.0f);
  pMap->MapAction(s_hRenderColliders, szSubPath, 2.0f);
  // pMap->MapAction(s_hRenderJoints, szSubPath, 3.0f);
  pMap->MapAction(s_hRenderSwingLimits, szSubPath, 4.0f);
  pMap->MapAction(s_hRenderTwistLimits, szSubPath, 5.0f);
  pMap->MapAction(s_hRenderPreviewMesh, szSubPath, 6.0f);
}

plSkeletonAction::plSkeletonAction(const plActionContext& context, const char* szName, plSkeletonAction::ActionType type)
  : plButtonAction(context, szName, false, "")
{
  m_Type = type;

  m_pSkeletonpDocument = const_cast<plSkeletonAssetDocument*>(static_cast<const plSkeletonAssetDocument*>(context.m_pDocument));
  m_pSkeletonpDocument->Events().AddEventHandler(plMakeDelegate(&plSkeletonAction::AssetEventHandler, this));

  switch (m_Type)
  {
    case ActionType::RenderBones:
      SetIconPath(":/EditorPluginAssets/SkeletonBones.svg");
      break;

    case ActionType::RenderColliders:
      SetIconPath(":/EditorPluginAssets/SkeletonColliders.svg");
      break;

    case ActionType::RenderJoints:
      SetIconPath(":/EditorPluginAssets/SkeletonJoints.svg");
      break;

    case ActionType::RenderSwingLimits:
      SetIconPath(":/EditorPluginAssets/JointSwingLimits.svg");
      break;

    case ActionType::RenderTwistLimits:
      SetIconPath(":/EditorPluginAssets/JointTwistLimits.svg");
      break;

    case ActionType::RenderPreviewMesh:
      SetIconPath(":/AssetIcons/PreviewMesh.svg");
      break;
  }

  UpdateState();
}

plSkeletonAction::~plSkeletonAction()
{
  m_pSkeletonpDocument->Events().RemoveEventHandler(plMakeDelegate(&plSkeletonAction::AssetEventHandler, this));
}

void plSkeletonAction::Execute(const plVariant& value)
{
  switch (m_Type)
  {
    case ActionType::RenderBones:
      m_pSkeletonpDocument->SetRenderBones(!m_pSkeletonpDocument->GetRenderBones());
      return;

    case ActionType::RenderColliders:
      m_pSkeletonpDocument->SetRenderColliders(!m_pSkeletonpDocument->GetRenderColliders());
      return;

    case ActionType::RenderJoints:
      m_pSkeletonpDocument->SetRenderJoints(!m_pSkeletonpDocument->GetRenderJoints());
      return;

    case ActionType::RenderSwingLimits:
      m_pSkeletonpDocument->SetRenderSwingLimits(!m_pSkeletonpDocument->GetRenderSwingLimits());
      return;

    case ActionType::RenderTwistLimits:
      m_pSkeletonpDocument->SetRenderTwistLimits(!m_pSkeletonpDocument->GetRenderTwistLimits());
      return;

    case ActionType::RenderPreviewMesh:
      m_pSkeletonpDocument->SetRenderPreviewMesh(!m_pSkeletonpDocument->GetRenderPreviewMesh());
      return;
  }
}

void plSkeletonAction::AssetEventHandler(const plSkeletonAssetEvent& e)
{
  switch (e.m_Type)
  {
    case plSkeletonAssetEvent::RenderStateChanged:
      UpdateState();
      break;
    default:
      break;
  }
}

void plSkeletonAction::UpdateState()
{
  if (m_Type == ActionType::RenderBones)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderBones());
  }

  if (m_Type == ActionType::RenderColliders)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderColliders());
  }

  if (m_Type == ActionType::RenderJoints)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderJoints());
  }

  if (m_Type == ActionType::RenderSwingLimits)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderSwingLimits());
  }

  if (m_Type == ActionType::RenderTwistLimits)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderTwistLimits());
  }

  if (m_Type == ActionType::RenderPreviewMesh)
  {
    SetCheckable(true);
    SetChecked(m_pSkeletonpDocument->GetRenderPreviewMesh());
  }
}
