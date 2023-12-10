#include <EditorPluginTypeScript/EditorPluginTypeScriptPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginTypeScript/DragDropHandlers/TypeScriptDragDropHandler.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTypeScriptComponentDragDropHandler, 1, plRTTIDefaultAllocator<plTypeScriptComponentDragDropHandler>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


float plTypeScriptComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "TypeScript") ? 1.0f : 0.0f;
}

void plTypeScriptComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "plTypeScriptComponent", "Script", GetAssetGuidString(pInfo), plUuid(), -1);
  }
  else
  {
    if (pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject("plTypeScriptComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
    {
      CreateDropObject(pInfo->m_vDropPosition, "plTypeScriptComponent", "Script", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
        pInfo->m_iTargetObjectInsertChildIndex);
    }
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
