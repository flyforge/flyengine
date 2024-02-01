#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/ComponentDragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <ToolsFoundation/Command/TreeCommands.h>


PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plComponentDragDropHandler, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

void plComponentDragDropHandler::CreateDropObject(const plVec3& vPosition, const char* szType, const char* szProperty, const plVariant& value, plUuid parent, plInt32 iInsertChildIndex)
{
  plVec3 vPos = vPosition;

  if (vPos.IsNaN())
    vPos.SetZero();

  plUuid ObjectGuid = plUuid::MakeUuid();

  plAddObjectCommand cmd;
  cmd.m_Parent = parent;
  cmd.m_Index = iInsertChildIndex;
  cmd.SetType("plGameObject");
  cmd.m_NewObjectGuid = ObjectGuid;
  cmd.m_sParentProperty = "Children";

  auto history = m_pDocument->GetCommandHistory();

  PL_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

  plSetObjectPropertyCommand cmd2;
  cmd2.m_Object = ObjectGuid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPos;
  PL_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");

  AttachComponentToObject(szType, szProperty, value, ObjectGuid);

  m_DraggedObjects.PushBack(ObjectGuid);
}

void plComponentDragDropHandler::AttachComponentToObject(const char* szType, const char* szProperty, const plVariant& value, plUuid ObjectGuid)
{
  auto history = m_pDocument->GetCommandHistory();

  plUuid CmpGuid = plUuid::MakeUuid();

  plAddObjectCommand cmd;

  cmd.SetType(szType);
  cmd.m_sParentProperty = "Components";
  cmd.m_Index = -1;
  cmd.m_NewObjectGuid = CmpGuid;
  cmd.m_Parent = ObjectGuid;
  PL_VERIFY(history->AddCommand(cmd).m_Result.Succeeded(), "AddCommand failed");

  if (value.IsA<plVariantArray>())
  {
    plResizeAndSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue = value.Get<plVariantArray>()[0];
    cmd2.m_Index = 0;
    PL_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
  }
  else
  {
    plSetObjectPropertyCommand cmd2;
    cmd2.m_Object = CmpGuid;
    cmd2.m_sProperty = szProperty;
    cmd2.m_NewValue = value;
    PL_VERIFY(history->AddCommand(cmd2).m_Result.Succeeded(), "AddCommand failed");
  }
}

void plComponentDragDropHandler::MoveObjectToPosition(const plUuid& guid, const plVec3& vPosition, const plQuat& qRotation)
{
  auto history = m_pDocument->GetCommandHistory();

  plSetObjectPropertyCommand cmd2;
  cmd2.m_Object = guid;

  cmd2.m_sProperty = "LocalPosition";
  cmd2.m_NewValue = vPosition;
  history->AddCommand(cmd2).AssertSuccess();

  if (qRotation.IsValid())
  {
    cmd2.m_sProperty = "LocalRotation";
    cmd2.m_NewValue = qRotation;
    history->AddCommand(cmd2).AssertSuccess();
  }
}

void plComponentDragDropHandler::MoveDraggedObjectsToPosition(plVec3 vPosition, bool bAllowSnap, const plVec3& normal)
{
  if (m_DraggedObjects.IsEmpty() || !vPosition.IsValid())
    return;

  if (bAllowSnap)
  {
    plSnapProvider::SnapTranslation(vPosition);
  }

  auto history = m_pDocument->GetCommandHistory();

  history->StartTransaction("Move to Position");

  plQuat rot;
  rot.SetIdentity();

  if (normal.IsValid() && !m_vAlignAxisWithNormal.IsZero(0.01f))
  {
    rot = plQuat::MakeShortestRotation(m_vAlignAxisWithNormal, normal);
  }

  for (const auto& guid : m_DraggedObjects)
  {
    MoveObjectToPosition(guid, vPosition, rot);
  }

  history->FinishTransaction();
}

void plComponentDragDropHandler::SelectCreatedObjects()
{
  plDeque<const plDocumentObject*> NewSel;
  for (const auto& id : m_DraggedObjects)
  {
    NewSel.PushBack(m_pDocument->GetObjectManager()->GetObject(id));
  }

  m_pDocument->GetSelectionManager()->SetSelection(NewSel);
}

void plComponentDragDropHandler::BeginTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->BeginTemporaryCommands("Adjust Objects");
}

void plComponentDragDropHandler::EndTemporaryCommands()
{
  m_pDocument->GetCommandHistory()->FinishTemporaryCommands();
}

void plComponentDragDropHandler::CancelTemporaryCommands()
{
  if (m_DraggedObjects.IsEmpty())
    return;

  m_pDocument->GetSelectionManager()->Clear();

  m_pDocument->GetCommandHistory()->CancelTemporaryCommands();
}

void plComponentDragDropHandler::OnDragBegin(const plDragDropInfo* pInfo)
{
  m_pDocument = plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);
  PL_ASSERT_DEV(m_pDocument != nullptr, "Invalid document GUID in drag & drop operation");

  m_pDocument->GetCommandHistory()->StartTransaction("Drag Object");
}

void plComponentDragDropHandler::OnDragUpdate(const plDragDropInfo* pInfo)
{
  plVec3 vPos = pInfo->m_vDropPosition;

  if (vPos.IsNaN() || !pInfo->m_TargetObject.IsValid())
    vPos.SetZero();

  plVec3 vNormal = pInfo->m_vDropNormal;

  if (!vNormal.IsValid() || vNormal.IsZero())
    vNormal = plVec3(1, 0, 0);

  MoveDraggedObjectsToPosition(vPos, !pInfo->m_bCtrlKeyDown, vNormal);
}

void plComponentDragDropHandler::OnDragCancel()
{
  CancelTemporaryCommands();
  m_pDocument->GetCommandHistory()->CancelTransaction();

  m_DraggedObjects.Clear();
}

void plComponentDragDropHandler::OnDrop(const plDragDropInfo* pInfo)
{
  EndTemporaryCommands();
  m_pDocument->GetCommandHistory()->FinishTransaction();

  SelectCreatedObjects();

  m_DraggedObjects.Clear();
}

float plComponentDragDropHandler::CanHandle(const plDragDropInfo* pInfo) const
{
  if (pInfo->m_sTargetContext != "viewport" && pInfo->m_sTargetContext != "scenetree")
    return 0.0f;

  const plDocument* pDocument = plDocumentManager::GetDocumentByGuid(pInfo->m_TargetDocument);

  const plRTTI* pRttiScene = plRTTI::FindTypeByName("plSceneDocument");

  if (pRttiScene == nullptr)
    return 0.0f;

  if (!pDocument->GetDynamicRTTI()->IsDerivedFrom(pRttiScene))
    return 0.0f;

  return 1.0f;
}
