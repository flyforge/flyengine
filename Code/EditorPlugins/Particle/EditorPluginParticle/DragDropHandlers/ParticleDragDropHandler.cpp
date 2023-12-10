#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginParticle/DragDropHandlers/ParticleDragDropHandler.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleComponentDragDropHandler, 1, plRTTIDefaultAllocator<plParticleComponentDragDropHandler>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


float plParticleComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Particle Effect") ? 1.0f : 0.0f;
}

void plParticleComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_sTargetContext == "viewport")
  {
    CreateDropObject(pInfo->m_vDropPosition, "plParticleComponent", "Effect", GetAssetGuidString(pInfo), plUuid(), -1);

    m_vAlignAxisWithNormal = plVec3::MakeAxisZ();
  }
  else
    CreateDropObject(pInfo->m_vDropPosition, "plParticleComponent", "Effect", GetAssetGuidString(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);

  SelectCreatedObjects();
  BeginTemporaryCommands();
}
