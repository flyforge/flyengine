#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

class plExposedParametersAttribute;
class plExposedParameterCommandAccessor;

/// \brief Default state provider handling variant maps with the plExposedParametersAttribute set. Reflects the default value defined in the plExposedParameter.
class PLASMA_EDITORFRAMEWORK_DLL plExposedParametersDefaultStateProvider : public plDefaultStateProvider
{
public:
  static plSharedPtr<plDefaultStateProvider> CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp);
  plExposedParametersDefaultStateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp);

  virtual plInt32 GetRootDepth() const override;
  virtual plColorGammaUB GetBackgroundColor() const override;
  virtual plString GetStateProviderName() const override { return "Exposed Parameters"; }

  virtual plVariant GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff) override;

  virtual bool IsDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus RevertProperty(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;

private:
  const plDocumentObject* m_pObject = nullptr;
  const plAbstractProperty* m_pProp = nullptr;
  const plExposedParametersAttribute* m_pAttrib = nullptr;
  const plAbstractProperty* m_pParameterSourceProp = nullptr;
};
