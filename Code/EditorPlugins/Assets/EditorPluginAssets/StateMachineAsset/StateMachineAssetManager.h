#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>

class plStateMachineAssetManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plStateMachineAssetManager, plAssetDocumentManager);

public:
  plStateMachineAssetManager();
  ~plStateMachineAssetManager();

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
