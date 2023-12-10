#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plSubstancePackageAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSubstancePackageAssetDocumentManager, plAssetDocumentManager);

public:
  plSubstancePackageAssetDocumentManager();
  ~plSubstancePackageAssetDocumentManager();

  const plAssetDocumentTypeDescriptor& GetTextureTypeDesc() const { return m_TextureTypeDesc; }

private:
  virtual void FillOutSubAssetList(const plAssetDocumentInfo& assetInfo, plDynamicArray<plSubAssetData>& out_subAssets) const override;
  virtual plString GetAssetTableEntry(const plSubAsset* pSubAsset, plStringView sDataDirectory, const plPlatformProfile* pAssetProfile) const override;
  virtual plUInt64 ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const override { return 1; }

  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  plAssetDocumentTypeDescriptor m_PackageTypeDesc;
  plAssetDocumentTypeDescriptor m_TextureTypeDesc;
};
