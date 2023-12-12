#pragma once

#include <ToolsFoundation/Object/ObjectAccessorBase.h>

class plDocumentObject;

class PLASMA_TOOLSFOUNDATION_DLL plObjectProxyAccessor : public plObjectAccessorBase
{
public:
  plObjectProxyAccessor(plObjectAccessorBase* pSource);
  virtual ~plObjectProxyAccessor();

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(const char* szDisplayString) override;
  virtual void CancelTransaction() override;
  virtual void FinishTransaction() override;
  virtual void BeginTemporaryCommands(const char* szDisplayString, bool bFireEventsWhenUndoingTempCommands = false) override;
  virtual void CancelTemporaryCommands() override;
  virtual void FinishTemporaryCommands() override;

  ///@}
  /// \name plObjectAccessorBase overrides
  ///@{

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

  ///@}

protected:
  plObjectAccessorBase* m_pSource = nullptr;
};
