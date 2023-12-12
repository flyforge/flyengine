#pragma once
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

class plPropertyAnimAssetDocument;
class plPropertyAnimObjectManager;

class plPropertyAnimObjectAccessor : public plObjectCommandAccessor
{
public:
  plPropertyAnimObjectAccessor(plPropertyAnimAssetDocument* pDoc, plCommandHistory* pHistory);

  virtual plStatus GetValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant& out_value, plVariant index = plVariant()) override;
  virtual plStatus SetValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;

  virtual plStatus InsertValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& newValue, plVariant index = plVariant()) override;
  virtual plStatus RemoveValue(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus MoveValue(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, const plVariant& oldIndex, const plVariant& newIndex) override;

  virtual plStatus AddObject(const plDocumentObject* pParent, const plAbstractProperty* pParentProp, const plVariant& index, const plRTTI* pType,
    plUuid& inout_objectGuid) override;
  virtual plStatus RemoveObject(const plDocumentObject* pObject) override;
  virtual plStatus MoveObject(
    const plDocumentObject* pObject, const plDocumentObject* pNewParent, const plAbstractProperty* pParentProp, const plVariant& index) override;

private:
  bool IsTemporary(const plDocumentObject* pObject) const;
  bool IsTemporary(const plDocumentObject* pParent, const plAbstractProperty* pParentProp) const;
  typedef plDelegate<void(const plUuid&)> OnAddTrack;
  plUuid FindOrAddTrack(
    const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target, OnAddTrack onAddTrack);

  plStatus SetCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plPropertyAnimTarget::Enum target,
    double fOldValue, double fNewValue);
  plStatus SetOrInsertCurveCp(const plUuid& track, double fValue);

  plStatus SetColorCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, const plColorGammaUB& oldValue,
    const plColorGammaUB& newValue);
  plStatus SetOrInsertColorCurveCp(const plUuid& track, const plColorGammaUB& value);

  plStatus SetAlphaCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, plUInt8 oldValue, plUInt8 newValue);
  plStatus SetOrInsertAlphaCurveCp(const plUuid& track, plUInt8 value);

  plStatus SetIntensityCurveCp(const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index, float oldValue, float newValue);
  plStatus SetOrInsertIntensityCurveCp(const plUuid& track, float value);

  void SeparateColor(const plColor& color, plColorGammaUB& gamma, plUInt8& alpha, float& intensity);

  plUniquePtr<plObjectAccessorBase> m_pObjAccessor;
  plPropertyAnimAssetDocument* m_pDocument = nullptr;
  plPropertyAnimObjectManager* m_pObjectManager = nullptr;
};
