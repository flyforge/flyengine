#pragma once

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

class plDocumentObjectManager;

class PLASMA_TOOLSFOUNDATION_DLL plObjectDirectAccessor : public plObjectAccessorBase
{
public:
  plObjectDirectAccessor(plDocumentObjectManager* pManager);

  virtual const plDocumentObject* GetObject(const plUuid& object) override;
  virtual plStatus GetValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index = plVariant()) override;
  virtual plStatus SetValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;
  virtual plStatus InsertValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;
  virtual plStatus RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus MoveValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex) override;
  virtual plStatus GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp, plInt32& out_iCount) override;

  virtual plStatus AddObject(const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType,
    plUuid& inout_objectGuid) override;
  virtual plStatus RemoveObject(const plDocumentObject* pObject) override;
  virtual plStatus MoveObject(
    const plDocumentObject* pObject, const plDocumentObject* pNewParent, const plAbstractProperty* pParentProp, const plVariant& index) override;

  virtual plStatus GetKeys(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_keys) override;
  virtual plStatus GetValues(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_values) override;

protected:
  plDocumentObjectManager* m_pManager;
};
