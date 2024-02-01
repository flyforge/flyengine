#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginProcGen/DragDropHandlers/ProcGenDragDropHandler.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcPlacementComponentDragDropHandler, 1, plRTTIDefaultAllocator<plProcPlacementComponentDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;


float plProcPlacementComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "ProcGen Graph") ? 1.0f : 0.0f;
}

void plProcPlacementComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  plUuid targetObject;
  plInt32 iTargetInsertChildIndex = -1;

  if (pInfo->m_sTargetContext != "viewport")
  {
    targetObject = pInfo->m_TargetObject;
    iTargetInsertChildIndex = pInfo->m_iTargetObjectInsertChildIndex;
  }

  CreateDropObject(pInfo->m_vDropPosition, "plProcPlacementComponent", "Resource", GetAssetGuidString(pInfo), targetObject, iTargetInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
