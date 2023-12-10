#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MeshDragDropHandler.h>

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshComponentDragDropHandler, 1, plRTTIDefaultAllocator<plMeshComponentDragDropHandler>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

float plMeshComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Mesh") ? 1.0f : 0.0f;
}

void plMeshComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "plMeshComponent", "Mesh", GetAssetGuidString(pInfo), plUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "plMeshComponent", "Mesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimatedMeshComponentDragDropHandler, 1, plRTTIDefaultAllocator<plAnimatedMeshComponentDragDropHandler>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

float plAnimatedMeshComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Animated Mesh") ? 1.0f : 0.0f;
}

void plAnimatedMeshComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "plAnimatedMeshComponent", "Mesh", GetAssetGuidString(pInfo), plUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "plAnimatedMeshComponent", "Mesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
