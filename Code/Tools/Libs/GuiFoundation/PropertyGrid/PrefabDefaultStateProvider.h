#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/DefaultState.h>

/// \brief Default state provider that reflects the default state defined in the prefab template.
class PLASMA_GUIFOUNDATION_DLL plPrefabDefaultStateProvider : public plDefaultStateProvider
{
public:
  static plSharedPtr<plDefaultStateProvider> CreateProvider(plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp);

  plPrefabDefaultStateProvider(const plUuid& rootObjectGuid, const plUuid& createFromPrefab, const plUuid& prefabSeedGuid, plInt32 iRootDepth);
  virtual plInt32 GetRootDepth() const override;
  virtual plColorGammaUB GetBackgroundColor() const override;
  virtual plString GetStateProviderName() const override { return "Prefab"; }

  virtual plVariant GetDefaultValue(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plVariant index = plVariant()) override;
  virtual plStatus CreateRevertContainerDiff(SuperArray superPtr, plObjectAccessorBase* pAccessor, const plDocumentObject* pObject, const plAbstractProperty* pProp, plDeque<plAbstractGraphDiffOperation>& out_diff) override;

private:
  const plUuid m_RootObjectGuid;
  const plUuid m_CreateFromPrefab;
  const plUuid m_PrefabSeedGuid;
  plInt32 m_iRootDepth = 0;
};
