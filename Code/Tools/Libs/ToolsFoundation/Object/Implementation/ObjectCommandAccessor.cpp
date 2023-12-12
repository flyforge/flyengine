#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

plObjectCommandAccessor::plObjectCommandAccessor(plCommandHistory* pHistory)
  : plObjectDirectAccessor(const_cast<plDocumentObjectManager*>(pHistory->GetDocument()->GetObjectManager()))
  , m_pHistory(pHistory)
{
}

void plObjectCommandAccessor::StartTransaction(const char* szDisplayString)
{
  m_pHistory->StartTransaction(szDisplayString);
}

void plObjectCommandAccessor::CancelTransaction()
{
  m_pHistory->CancelTransaction();
}

void plObjectCommandAccessor::FinishTransaction()
{
  m_pHistory->FinishTransaction();
}

void plObjectCommandAccessor::BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pHistory->BeginTemporaryCommands(szDisplayString, bFireEventsWhenUndoingTempCommands);
}

void plObjectCommandAccessor::CancelTemporaryCommands()
{
  m_pHistory->CancelTemporaryCommands();
}

void plObjectCommandAccessor::FinishTemporaryCommands()
{
  m_pHistory->FinishTemporaryCommands();
}

plStatus plObjectCommandAccessor::SetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  plSetObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_NewValue = newValue;
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

plStatus plObjectCommandAccessor::InsertValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  plInsertObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_NewValue = newValue;
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

plStatus plObjectCommandAccessor::RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index /*= plVariant()*/)
{
  plRemoveObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_Index = index;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

plStatus plObjectCommandAccessor::MoveValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex)
{
  plMoveObjectPropertyCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  cmd.m_OldIndex = oldIndex;
  cmd.m_NewIndex = newIndex;
  cmd.m_sProperty = pProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}

plStatus plObjectCommandAccessor::AddObject(
  const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid)
{
  plAddObjectCommand cmd;
  cmd.m_Parent = pParent ? pParent->GetGuid() : plUuid();
  cmd.m_Index = index;
  cmd.m_pType = pType;
  cmd.m_NewObjectGuid = inout_objectGuid;
  cmd.m_sParentProperty = pParentProp ? pParentProp->GetPropertyName() : "Children";
  plStatus res = m_pHistory->AddCommand(cmd);
  if (res.m_Result.Succeeded())
    inout_objectGuid = cmd.m_NewObjectGuid;
  return res;
}

plStatus plObjectCommandAccessor::RemoveObject(const plDocumentObject* pObject)
{
  plRemoveObjectCommand cmd;
  cmd.m_Object = pObject->GetGuid();
  return m_pHistory->AddCommand(cmd);
}

plStatus plObjectCommandAccessor::MoveObject(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, const plAbstractProperty* pParentProp, const plVariant& index)
{
  plMoveObjectCommand cmd;
  cmd.m_NewParent = pNewParent ? pNewParent->GetGuid() : plUuid();
  cmd.m_Object = pObject->GetGuid();
  cmd.m_Index = index;
  cmd.m_sParentProperty = pParentProp->GetPropertyName();
  return m_pHistory->AddCommand(cmd);
}
