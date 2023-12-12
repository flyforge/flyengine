#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

void plObjectAccessorBase::StartTransaction(const char* szDisplayString) {}


void plObjectAccessorBase::CancelTransaction() {}


void plObjectAccessorBase::FinishTransaction() {}


void plObjectAccessorBase::BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/) {}


void plObjectAccessorBase::CancelTemporaryCommands() {}


void plObjectAccessorBase::FinishTemporaryCommands() {}


plStatus plObjectAccessorBase::GetValue(const plDocumentObject* pObject, const char* szProp, plVariant& out_value, plVariant index /*= plVariant()*/)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetValue(pObject, pProp, out_value, index);
}


plStatus plObjectAccessorBase::SetValue(
  const plDocumentObject* pObject, const char* szProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return SetValue(pObject, pProp, newValue, index);
}


plStatus plObjectAccessorBase::InsertValue(
  const plDocumentObject* pObject, const char* szProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return InsertValue(pObject, pProp, newValue, index);
}


plStatus plObjectAccessorBase::RemoveValue(const plDocumentObject* pObject, const char* szProp, plVariant index /*= plVariant()*/)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return RemoveValue(pObject, pProp, index);
}


plStatus plObjectAccessorBase::MoveValue(const plDocumentObject* pObject, const char* szProp, const plVariant& oldIndex, const plVariant& newIndex)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return MoveValue(pObject, pProp, oldIndex, newIndex);
}


plStatus plObjectAccessorBase::GetCount(const plDocumentObject* pObject, const char* szProp, plInt32& out_iCount)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetCount(pObject, pProp, out_iCount);
}


plStatus plObjectAccessorBase::AddObject(
  const plDocumentObject* pParent, const char* szParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid)
{
  const plAbstractProperty* pProp = pParent->GetType()->FindPropertyByName(szParentProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szParentProp, pParent->GetType()->GetTypeName()));
  return AddObject(pParent, pProp, index, pType, inout_objectGuid);
}

plStatus plObjectAccessorBase::MoveObject(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, const char* szParentProp, const plVariant& index)
{
  const plAbstractProperty* pProp = pNewParent->GetType()->FindPropertyByName(szParentProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szParentProp, pNewParent->GetType()->GetTypeName()));
  return MoveObject(pObject, pNewParent, pProp, index);
}


plStatus plObjectAccessorBase::GetKeys(const plDocumentObject* pObject, const char* szProp, plDynamicArray<plVariant>& out_keys)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetKeys(pObject, pProp, out_keys);
}


plStatus plObjectAccessorBase::GetValues(const plDocumentObject* pObject, const char* szProp, plDynamicArray<plVariant>& out_values)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));
  return GetValues(pObject, pProp, out_values);
}

const plDocumentObject* plObjectAccessorBase::GetChildObject(const plDocumentObject* pObject, const char* szProp, plVariant index)
{
  plVariant value;
  if (GetValue(pObject, szProp, value, index).Succeeded() && value.IsA<plUuid>())
  {
    return GetObject(value.Get<plUuid>());
  }
  return nullptr;
}

plStatus plObjectAccessorBase::Clear(const plDocumentObject* pObject, const char* szProp)
{
  const plAbstractProperty* pProp = pObject->GetType()->FindPropertyByName(szProp);
  if (!pProp)
    return plStatus(plFmt("The property '{0}' does not exist in type '{1}'.", szProp, pObject->GetType()->GetTypeName()));

  plHybridArray<plVariant, 8> keys;
  plStatus res = GetKeys(pObject, pProp, keys);
  if (res.Failed())
    return res;

  for (plInt32 i = keys.GetCount() - 1; i >= 0; --i)
  {
    res = RemoveValue(pObject, pProp, keys[i]);
    if (res.Failed())
      return res;
  }
  return plStatus(PLASMA_SUCCESS);
}

plObjectAccessorBase::plObjectAccessorBase(const plDocumentObjectManager* pManager)
  : m_pConstManager(pManager)
{
}

plObjectAccessorBase::~plObjectAccessorBase() {}

const plDocumentObjectManager* plObjectAccessorBase::GetObjectManager() const
{
  return m_pConstManager;
}

void plObjectAccessorBase::FireDocumentObjectStructureEvent(const plDocumentObjectStructureEvent& e)
{
  m_pConstManager->m_StructureEvents.Broadcast(e);
}

void plObjectAccessorBase::FireDocumentObjectPropertyEvent(const plDocumentObjectPropertyEvent& e)
{
  m_pConstManager->m_PropertyEvents.Broadcast(e);
}
