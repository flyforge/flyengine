#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// \brief This is the fall back default state provider which handles the default state set via the plDefaultAttribute on the reflected type.
class PLASMA_GUIFOUNDATION_DLL plAttributeDefaultStateProvider : public plDefaultStateProvider
{
public:
  static plSharedPtr<plDefaultStateProvider> CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp);

  virtual plInt32 GetRootDepth() const override;
  virtual plColorGammaUB GetBackgroundColor() const override;
  virtual plString GetStateProviderName() const override { return "Attribute"; }

  virtual plVariant GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff) override;
};
