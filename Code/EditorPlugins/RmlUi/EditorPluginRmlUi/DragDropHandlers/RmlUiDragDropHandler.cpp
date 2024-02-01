#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginRmlUi/DragDropHandlers/RmlUiDragDropHandler.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiComponentDragDropHandler, 1, plRTTIDefaultAllocator<plRmlUiComponentDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;


float plRmlUiComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "RmlUi") ? 1.0f : 0.0f;
}

void plRmlUiComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "plRmlUiCanvas2DComponent", "RmlFile", GetAssetGuidString(pInfo), plUuid(), -1);
  }
  else
  {
    CreateDropObject(pInfo->m_vDropPosition, "plRmlUiCanvas2DComponent", "RmlFile", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
      pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
