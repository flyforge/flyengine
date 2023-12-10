#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginJolt/DragDropHandlers/JoltCollisionMeshDragDropHandler.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plJoltCollisionMeshComponentDragDropHandler, 1, plRTTIDefaultAllocator<plJoltCollisionMeshComponentDragDropHandler>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


float plJoltCollisionMeshComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return (IsSpecificAssetType(pInfo, "Jolt_Colmesh_Triangle") || IsSpecificAssetType(pInfo, "Jolt_Colmesh_Convex")) ? 1.0f : 0.0f;
}

void plJoltCollisionMeshComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
    CreateDropObject(pInfo->m_vDropPosition, "plJoltStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), plUuid(), -1);
  else
  {
    if (pInfo->m_iTargetObjectInsertChildIndex == -1) // dropped directly on a node -> attach component only
    {
      AttachComponentToObject("plJoltStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject);

      // make sure this object gets selected
      m_DraggedObjects.PushBack(pInfo->m_TargetObject);
    }
    else
      CreateDropObject(pInfo->m_vDropPosition, "plJoltStaticActorComponent", "CollisionMesh", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
        pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
