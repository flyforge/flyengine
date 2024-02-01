#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class PL_TOOLSFOUNDATION_DLL plNodeCommandAccessor : public plObjectCommandAccessor
{
public:
  plNodeCommandAccessor(plCommandHistory* pHistory);
  ~plNodeCommandAccessor();

  virtual plStatus SetValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;

  virtual plStatus InsertValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;
  virtual plStatus RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus MoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex) override;

  virtual plStatus AddObject(const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType, plUuid& inout_objectGuid) override;
  virtual plStatus RemoveObject(const plDocumentObject* pObject) override;

private:
  bool IsNode(const plDocumentObject* pObject) const;
  bool IsDynamicPinProperty(const plDocumentObject* pObject, const plAbstractProperty* pProp) const;

  struct ConnectionInfo
  {
    const plDocumentObject* m_pSource = nullptr;
    const plDocumentObject* m_pTarget = nullptr;
    plString m_sSourcePin;
    plString m_sTargetPin;
  };

  plStatus DisconnectAllPins(const plDocumentObject* pObject, plDynamicArray<ConnectionInfo>& out_oldConnections);
  plStatus TryReconnectAllPins(const plDocumentObject* pObject, const plDynamicArray<ConnectionInfo>& oldConnections);
};
