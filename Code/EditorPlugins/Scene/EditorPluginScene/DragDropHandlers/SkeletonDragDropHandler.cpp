#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/SkeletonDragDropHandler.h>

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkeletonComponentDragDropHandler, 1, plRTTIDefaultAllocator<plSkeletonComponentDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;

float plSkeletonComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Skeleton") ? 1.0f : 0.0f;
}

void plSkeletonComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "plSkeletonComponent", "Skeleton", GetAssetGuidString(pInfo), plUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "plSkeletonComponent", "Skeleton", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
