#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginKraut/DragDropHandlers/KrautTreeDragDropHandler.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeComponentDragDropHandler, 1, plRTTIDefaultAllocator<plKrautTreeComponentDragDropHandler>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


float plKrautTreeComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Kraut Tree") ? 1.0f : 0.0f;
}

void plKrautTreeComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "plKrautTreeComponent", "KrautTree", GetAssetGuidString(pInfo), plUuid(), -1);
  else
    CreateDropObject(pInfo->m_vDropPosition, "plKrautTreeComponent", "KrautTree", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
      pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
