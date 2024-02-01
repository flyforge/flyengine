#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class plDynamicDefaultValueAttribute;
class plPropertyPath;

/// \brief Retrieves the dynamic default state of an object or container attributed with plDynamicDefaultValueAttribute from an asset's meta data.
class PL_EDITORFRAMEWORK_DLL plDynamicDefaultStateProvider : public plDefaultStateProvider
{
public:
  static plSharedPtr<plDefaultStateProvider> CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp);

  plDynamicDefaultStateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plDocumentObject* pClassObject, const plDocumentObject* pRootObject, const plAbstractProperty* pRootProp, plInt32 iRootDepth);

  virtual plInt32 GetRootDepth() const override;
  virtual plColorGammaUB GetBackgroundColor() const override;
  virtual plString GetStateProviderName() const override { return "Dynamic"; }

  virtual plVariant GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff) override;

private:
  const plReflectedClass* GetMetaInfo(plObjectAccessorBase* pAccessor) const;
  const plResult CreatePath(plObjectAccessorBase* pAccessor, const plReflectedClass* pMeta, plPropertyPath& propertyPath, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant());

  const plDocumentObject* m_pObject = nullptr;
  const plDocumentObject* m_pClassObject = nullptr;
  const plDocumentObject* m_pRootObject = nullptr;
  const plAbstractProperty* m_pRootProp = nullptr;
  plInt32 m_iRootDepth = 0;
  const plDynamicDefaultValueAttribute* m_pAttrib = nullptr;
  const plAbstractProperty* m_pClassSourceProp = nullptr;
  const plRTTI* m_pClassType = nullptr;
  const plAbstractProperty* m_pClassProperty = nullptr;
};
