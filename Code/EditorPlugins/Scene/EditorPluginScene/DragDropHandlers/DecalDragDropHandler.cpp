#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/DecalDragDropHandler.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalComponentDragDropHandler, 1, plRTTIDefaultAllocator<plDecalComponentDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;


float plDecalComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Decal") ? 1.0f : 0.0f;
}

void plDecalComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  plVariantArray var;
  var.PushBack(GetAssetGuidString(pInfo));

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "plDecalComponent", "Decals", var, plUuid(), -1);

    m_vAlignAxisWithNormal = -plVec3::MakeAxisX();
  }
  else
    CreateDropObject(pInfo->m_vDropPosition, "plDecalComponent", "Decals", var, pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
