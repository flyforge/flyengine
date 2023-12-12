#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

plObjectProxyAccessor::plObjectProxyAccessor(plObjectAccessorBase* pSource)
  : plObjectAccessorBase(pSource->GetObjectManager())
  , m_pSource(pSource)
{
}

plObjectProxyAccessor::~plObjectProxyAccessor() {}

void plObjectProxyAccessor::StartTransaction(const char* szDisplayString)
{
  m_pSource->StartTransaction(szDisplayString);
}

void plObjectProxyAccessor::CancelTransaction()
{
  m_pSource->CancelTransaction();
}

void plObjectProxyAccessor::FinishTransaction()
{
  m_pSource->FinishTransaction();
}

void plObjectProxyAccessor::BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands /*= false*/)
{
  m_pSource->BeginTemporaryCommands(szDisplayString, bFireEventsWhenUndoingTempCommands);
}

void plObjectProxyAccessor::CancelTemporaryCommands()
{
  m_pSource->CancelTemporaryCommands();
}

void plObjectProxyAccessor::FinishTemporaryCommands()
{
  m_pSource->FinishTemporaryCommands();
}

const plDocumentObject* plObjectProxyAccessor::GetObject(const plUuid& object)
{
  return m_pSource->GetObject(object);
}

plStatus plObjectProxyAccessor::GetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index /*= plVariant()*/)
{
  return m_pSource->GetValue(pObject, pProp, out_value, index);
}

plStatus plObjectProxyAccessor::SetValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  return m_pSource->SetValue(pObject, pProp, newValue, index);
}

plStatus plObjectProxyAccessor::InsertValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index /*= plVariant()*/)
{
  return m_pSource->InsertValue(pObject, pProp, newValue, index);
}

plStatus plObjectProxyAccessor::RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index /*= plVariant()*/)
{
  return m_pSource->RemoveValue(pObject, pProp, index);
}

plStatus plObjectProxyAccessor::MoveValue(
  const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex)
{
  return m_pSource->MoveValue(pObject, pProp, oldIndex, newIndex);
}

plStatus plObjectProxyAccessor::GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp, plInt32& out_iCount)
{
  return m_pSource->GetCount(pObject, pProp, out_iCount);
}

plStatus plObjectProxyAccessor::AddObject(
  const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid)
{
  return m_pSource->AddObject(pParent, pParentProp, index, pType, inout_objectGuid);
}

plStatus plObjectProxyAccessor::RemoveObject(const plDocumentObject* pObject)
{
  return m_pSource->RemoveObject(pObject);
}

plStatus plObjectProxyAccessor::MoveObject(
  const plDocumentObject* pObject, const plDocumentObject* pNewParent, const plAbstractProperty* pParentProp, const plVariant& index)
{
  return m_pSource->MoveObject(pObject, pNewParent, pParentProp, index);
}

plStatus plObjectProxyAccessor::GetKeys(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_keys)
{
  return m_pSource->GetKeys(pObject, pProp, out_keys);
}

plStatus plObjectProxyAccessor::GetValues(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_values)
{
  return m_pSource->GetValues(pObject, pProp, out_values);
}
