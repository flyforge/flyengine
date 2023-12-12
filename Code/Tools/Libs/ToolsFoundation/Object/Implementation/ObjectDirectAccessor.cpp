#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/ObjectDirectAccessor.h>

plObjectDirectAccessor::plObjectDirectAccessor(plDocumentObjectManager* pManager)
  : plObjectAccessorBase(pManager)
  , m_pManager(pManager)
{
}

const plDocumentObject* plObjectDirectAccessor::GetObject(const plUuid& object)
{
  return m_pManager->GetObject(object);
}

plStatus plObjectDirectAccessor::GetValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index)
{
  if (pProp == nullptr)
    return plStatus("Property is null.");

  plStatus res;
  out_value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), index, &res);
  return res;
}

plStatus plObjectDirectAccessor::SetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index)
{
  plDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().SetValue(pProp->GetPropertyName(), newValue, index);
  return plStatus(bRes ? PLASMA_SUCCESS : PLASMA_FAILURE);
}

plStatus plObjectDirectAccessor::InsertValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index)
{
  plDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), index, newValue);
  return plStatus(bRes ? PLASMA_SUCCESS : PLASMA_FAILURE);
}

plStatus plObjectDirectAccessor::RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index)
{
  plDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), index);
  return plStatus(bRes ? PLASMA_SUCCESS : PLASMA_FAILURE);
}

plStatus plObjectDirectAccessor::MoveValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex)
{
  plDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  bool bRes = pObj->GetTypeAccessor().MoveValue(pProp->GetPropertyName(), oldIndex, newIndex);
  return plStatus(bRes ? PLASMA_SUCCESS : PLASMA_FAILURE);
}

plStatus plObjectDirectAccessor::GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp, plInt32& out_iCount)
{
  out_iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
  return plStatus(PLASMA_SUCCESS);
}

plStatus plObjectDirectAccessor::AddObject(
  const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid)
{
  PLASMA_SUCCEED_OR_RETURN(m_pManager->CanAdd(pType, pParent, pParentProp->GetPropertyName(), index));

  plDocumentObject* pPar = m_pManager->GetObject(pParent->GetGuid());
  PLASMA_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  if (!inout_objectGuid.IsValid())
    inout_objectGuid.CreateNewUuid();
  plDocumentObject* pObj = m_pManager->CreateObject(pType, inout_objectGuid);
  m_pManager->AddObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plObjectDirectAccessor::RemoveObject(const plDocumentObject* pObject)
{
  PLASMA_SUCCEED_OR_RETURN(m_pManager->CanRemove(pObject));

  plDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  m_pManager->RemoveObject(pObj);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plObjectDirectAccessor::MoveObject(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, const plAbstractProperty* pParentProp, const plVariant& index)
{
  PLASMA_SUCCEED_OR_RETURN(m_pManager->CanMove(pObject, pNewParent, pParentProp->GetPropertyName(), index));

  plDocumentObject* pObj = m_pManager->GetObject(pObject->GetGuid());
  PLASMA_ASSERT_DEBUG(pObj, "Object is not part of this document manager.");
  plDocumentObject* pPar = m_pManager->GetObject(pNewParent->GetGuid());
  PLASMA_ASSERT_DEBUG(pPar, "Parent is not part of this document manager.");

  m_pManager->MoveObject(pObj, pPar, pParentProp->GetPropertyName(), index);
  return plStatus(PLASMA_SUCCESS);
}

plStatus plObjectDirectAccessor::GetKeys(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_keys)
{
  bool bRes = pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), out_keys);
  return plStatus(bRes ? PLASMA_SUCCESS : PLASMA_FAILURE);
}

plStatus plObjectDirectAccessor::GetValues(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_values)
{
  bool bRes = pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), out_values);
  return plStatus(bRes ? PLASMA_SUCCESS : PLASMA_FAILURE);
}
