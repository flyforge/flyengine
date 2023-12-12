#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>

class plVisualScriptClassAssetManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plVisualScriptClassAssetManager, plAssetDocumentManager);

public:
  plVisualScriptClassAssetManager();
  ~plVisualScriptClassAssetManager();

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
