#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/VisualShader/VisualShaderActions.h>
#include <GuiFoundation/Action/ActionMapManager.h>

plActionDescriptorHandle plVisualShaderActions::s_hCleanGraph;

void plVisualShaderActions::RegisterActions()
{
  s_hCleanGraph = PL_REGISTER_ACTION_0("VisualShader.CleanGraph", plActionScope::Document, "Visual Shader", "", plVisualShaderAction);
}

void plVisualShaderActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hCleanGraph);
}

void plVisualShaderActions::MapActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hCleanGraph, "", 30.0f);
}


PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plVisualShaderAction, 0, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plVisualShaderAction::plVisualShaderAction(const plActionContext& context, const char* szName)
  : plButtonAction(context, szName, false, "")
{
  SetIconPath(":/EditorPluginAssets/Cleanup.svg");
}

plVisualShaderAction::~plVisualShaderAction() = default;

void plVisualShaderAction::Execute(const plVariant& value)
{
  plMaterialAssetDocument* pMaterial = (plMaterialAssetDocument*)(m_Context.m_pDocument);

  pMaterial->RemoveDisconnectedNodes();
}
