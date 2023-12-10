#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class PLASMA_TOOLSFOUNDATION_DLL plNodeCommandAccessor : public plObjectCommandAccessor
{
public:
  plNodeCommandAccessor(plCommandHistory* pHistory);
  ~plNodeCommandAccessor();

  virtual plStatus SetValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;

  virtual plStatus InsertValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;
  virtual plStatus RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus MoveValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex) override;

private:
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
