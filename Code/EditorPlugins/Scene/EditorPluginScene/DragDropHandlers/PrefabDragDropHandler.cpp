#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/DragDropHandlers/PrefabDragDropHandler.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabCache.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPrefabComponentDragDropHandler, 1, plRTTIDefaultAllocator<plPrefabComponentDragDropHandler>)
PL_END_DYNAMIC_REFLECTED_TYPE;


float plPrefabComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (plComponentDragDropHandler::CanHandle(pInfo) == 0.0f)
    return 0.0f;

  return IsSpecificAssetType(pInfo, "Prefab") ? 1.0f : 0.0f;
}

void plPrefabComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragBegin(pInfo);

  if (pInfo->m_bShiftKeyDown)
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), plUuid(), -1);
    else
      CreatePrefab(pInfo->m_vDropPosition, GetAssetGuid(pInfo), pInfo->m_TargetObject, pInfo->m_iTargetObjectInsertChildIndex);
  }
  else
  {
    if (pInfo->m_sTargetContext == "viewport")
      CreateDropObject(pInfo->m_vDropPosition, "plPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), plUuid(), -1);
    else
      CreateDropObject(pInfo->m_vDropPosition, "plPrefabReferenceComponent", "Prefab", GetAssetGuidString(pInfo), pInfo->m_TargetObject,
        pInfo->m_iTargetObjectInsertChildIndex);
  }

  SelectCreatedObjects();
  BeginTemporaryCommands();
}

void plPrefabComponentDragDropHandler::CreatePrefab(const plVec3& vPosition, const plUuid& AssetGuid, plUuid parent, plInt32 iInsertChildIndex)
{
  plVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  auto pCmdHistory = m_pDocument->GetCommandHistory();

  plInstantiatePrefabCommand PasteCmd;
  PasteCmd.m_Parent = parent;
  PasteCmd.m_CreateFromPrefab = AssetGuid;
  PasteCmd.m_Index = iInsertChildIndex;
  PasteCmd.m_sBasePrefabGraph = plPrefabCache::GetSingleton()->GetCachedPrefabDocument(AssetGuid);
  PasteCmd.m_RemapGuid = plUuid::MakeUuid();

  if (PasteCmd.m_sBasePrefabGraph.IsEmpty())
    return; // error

  pCmdHistory->AddCommand(PasteCmd).AssertSuccess();

  if (PasteCmd.m_CreatedRootObject.IsValid())
  {
    MoveObjectToPosition(PasteCmd.m_CreatedRootObject, vPos, plQuat::MakeIdentity());

    m_DraggedObjects.PushBack(PasteCmd.m_CreatedRootObject);
  }
}

void plPrefabComponentDragDropHandler::OnDragUpdate(const plDragDropInfo* pInfo)
{
  plComponentDragDropHandler::OnDragUpdate(pInfo);

  // the way prefabs are instantiated on the runtime side means the selection is not always immediately 'correct'
  // by resetting the selection, we can fix this
  SelectCreatedObjects();
}
