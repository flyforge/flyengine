#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/MaterialDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GameEngine/Gameplay/GreyBoxComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>
#include <ToolsFoundation/Command/TreeCommands.h>


PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMaterialDragDropHandler, 1, plRTTIDefaultAllocator<plMaterialDragDropHandler>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

void plMaterialDragDropHandler::RequestConfiguration(plDragDropConfig* pConfigToFillOut)
{
  pConfigToFillOut->m_bPickSelectedObjects = true;
}

float plMaterialDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport")
    return 0.0f;

  const plDocument* pDocument = plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom<plSceneDocument>())
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Material") ? 1.0f : 0.0f;
}

void plMaterialDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  m_pDocument = plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  PLASMA_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Drag Material", true);
}

void plMaterialDragDropHandler::OnDragUpdate(const plDragDropInfo* pInfo)
{
  if (!pInfo->m_TargetComponent.IsValid())
    return;

  const plDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

  if (!pComponent)
    return;

  if (m_AppliedToComponent == pInfo->m_TargetComponent && m_iAppliedToSlot == pInfo->m_iTargetObjectSubID)
    return;

  m_AppliedToComponent = pInfo->m_TargetComponent;
  m_iAppliedToSlot = pInfo->m_iTargetObjectSubID;

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<plMeshComponent>())
  {
    plResizeAndSetObjectPropertyCommand cmd;
    cmd.m_Object = pInfo->m_TargetComponent;
    cmd.m_Index = pInfo->m_iTargetObjectSubID;
    cmd.m_sProperty = "Materials";
    cmd.m_NewValue = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd).IgnoreResult();
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }

  if (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<plGreyBoxComponent>())
  {
    plSetObjectPropertyCommand cmd;
    cmd.m_Object = pInfo->m_TargetComponent;
    cmd.m_sProperty = "Material";
    cmd.m_NewValue = GetAssetGuidString(pInfo);

    m_pDocument->GetCommandHistory()->StartTransaction("Assign Material");
    m_pDocument->GetCommandHistory()->AddCommand(cmd).IgnoreResult();
    m_pDocument->GetCommandHistory()->FinishTransaction();
  }
}

void plMaterialDragDropHandler::OnDragCancel()
{
  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void plMaterialDragDropHandler::OnDrop(const plDragDropInfo* pInfo)
{
  if (pInfo->m_TargetComponent.IsValid())
  {
    const plDocumentObject* pComponent = m_pDocument->GetObjectManager()->GetObject(pInfo->m_TargetComponent);

    if (pComponent && (pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<plMeshComponent>() ||
                        pComponent->GetTypeAccessor().GetType()->IsDerivedFrom<plGreyBoxComponent>()))
    {
      m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
      return;
    }
  }

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}
