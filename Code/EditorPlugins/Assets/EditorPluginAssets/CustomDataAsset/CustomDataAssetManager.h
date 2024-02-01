#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plCustomDataAssetDocumentManager : public plAssetDocumentManager
{
  PL_ADD_DYNAMIC_REFLECTION(plCustomDataAssetDocumentManager, plAssetDocumentManager);

public:
  plCustomDataAssetDocumentManager();
  ~plCustomDataAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override
  {
    // CustomData structs are typically defined in plugins, which may have changed, so they are a candidate for clearing them from the asset cache
    return plAssetDocumentManager::OutputReliability::Unknown;
  }

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
