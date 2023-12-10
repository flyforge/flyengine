#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plColorGradientAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plColorGradientAssetDocumentManager, plAssetDocumentManager);

public:
  plColorGradientAssetDocumentManager();
  ~plColorGradientAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return plAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
