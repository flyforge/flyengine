#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plMeshAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshAssetDocumentManager, plAssetDocumentManager);

public:
  plMeshAssetDocumentManager();
  ~plMeshAssetDocumentManager();

  virtual plResult OpenPickedDocument(const plDocumentObject* pPickedComponent, plUInt32 uiPartIndex) override;

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
