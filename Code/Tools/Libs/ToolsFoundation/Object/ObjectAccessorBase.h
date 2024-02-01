#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plDocumentObject;

class PL_TOOLSFOUNDATION_DLL plObjectAccessorBase
{
public:
  virtual ~plObjectAccessorBase();
  const plDocumentObjectManager* GetObjectManager() const;

  /// \name Transaction Operations
  ///@{

  virtual void StartTransaction(plStringView sDisplayString);
  virtual void CancelTransaction();
  virtual void FinishTransaction();
  virtual void BeginTemporaryCommands(plStringView sDisplayString, bool bFireEventsWhenUndoingTempCommands = false);
  virtual void CancelTemporaryCommands();
  virtual void FinishTemporaryCommands();

  ///@}
  /// \name Object Access Interface
  ///@{

  virtual const plDocumentObject* GetObject(const plUuid& object) = 0;
  virtual plStatus GetValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index = plVariant()) = 0;
  virtual plStatus SetValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) = 0;
  virtual plStatus InsertValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) = 0;
  virtual plStatus RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) = 0;
  virtual plStatus MoveValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex) = 0;
  virtual plStatus GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp, plInt32& out_iCount) = 0;

  virtual plStatus AddObject(const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType,
    plUuid& inout_objectGuid) = 0;
  virtual plStatus RemoveObject(const plDocumentObject* pObject) = 0;
  virtual plStatus MoveObject(
    const plDocumentObject* pObject, const plDocumentObject* pNewParent, const plAbstractProperty* pParentProp, const plVariant& index) = 0;

  virtual plStatus GetKeys(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_keys) = 0;
  virtual plStatus GetValues(const plDocumentObject* pObject, const plAbstractProperty* pProp, plDynamicArray<plVariant>& out_values) = 0;

  ///@}
  /// \name Object Access Convenience Functions
  ///@{

  plStatus GetValue(const plDocumentObject* pObject, plStringView sProp, plVariant& out_value, plVariant index = plVariant());
  plStatus SetValue(const plDocumentObject* pObject, plStringView sProp, const plVariant& newValue, plVariant index = plVariant());
  plStatus InsertValue(const plDocumentObject* pObject, plStringView sProp, const plVariant& newValue, plVariant index = plVariant());
  plStatus RemoveValue(const plDocumentObject* pObject, plStringView sProp, plVariant index = plVariant());
  plStatus MoveValue(const plDocumentObject* pObject, plStringView sProp, const plVariant& oldIndex, const plVariant& newIndex);
  plStatus GetCount(const plDocumentObject* pObject, plStringView sProp, plInt32& out_iCount);

  plStatus AddObject(
    const plDocumentObject* pParent, plStringView sParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid);
  plStatus MoveObject(const plDocumentObject* pObject, const plDocumentObject* pNewParent, plStringView sParentProp, const plVariant& index);

  plStatus GetKeys(const plDocumentObject* pObject, plStringView sProp, plDynamicArray<plVariant>& out_keys);
  plStatus GetValues(const plDocumentObject* pObject, plStringView sProp, plDynamicArray<plVariant>& out_values);
  const plDocumentObject* GetChildObject(const plDocumentObject* pObject, plStringView sProp, plVariant index);

  plStatus Clear(const plDocumentObject* pObject, plStringView sProp);

  template <typename T>
  T Get(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant());
  template <typename T>
  T Get(const plDocumentObject* pObject, plStringView sProp, plVariant index = plVariant());
  plInt32 GetCount(const plDocumentObject* pObject, const plAbstractProperty* pProp);
  plInt32 GetCount(const plDocumentObject* pObject, plStringView sProp);

  ///@}

protected:
  plObjectAccessorBase(const plDocumentObjectManager* pManager);
  void FireDocumentObjectStructureEvent(const plDocumentObjectStructureEvent& e);
  void FireDocumentObjectPropertyEvent(const plDocumentObjectPropertyEvent& e);

protected:
  const plDocumentObjectManager* m_pConstManager;
};

#include <ToolsFoundation/Object/Implementation/ObjectAccessorBase_inl.h>
