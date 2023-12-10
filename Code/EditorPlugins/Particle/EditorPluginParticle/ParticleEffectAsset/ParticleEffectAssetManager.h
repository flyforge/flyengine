#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plParticleEffectAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleEffectAssetDocumentManager, plAssetDocumentManager);

public:
  plParticleEffectAssetDocumentManager();
  ~plParticleEffectAssetDocumentManager();


private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};

