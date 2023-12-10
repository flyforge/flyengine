#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plMaterialAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMaterialAssetDocumentManager, plAssetDocumentManager);

public:
  plMaterialAssetDocumentManager();
  ~plMaterialAssetDocumentManager();

  virtual plString GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDescriptor, plStringView sDataDirectory, plStringView sDocumentPath, plStringView sOutputTag, const plPlatformProfile* pAssetProfile) const override;
  virtual bool IsOutputUpToDate(plStringView sDocumentPath, plStringView sOutputTag, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor) override;

  static const char* const s_szShaderOutputTag;

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
