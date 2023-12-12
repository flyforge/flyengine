#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plMaterialAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMaterialAssetDocumentManager, plAssetDocumentManager);

public:
  plMaterialAssetDocumentManager();
  ~plMaterialAssetDocumentManager();

  virtual plString GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDescriptor, const char* szDataDirectory,
    const char* szDocumentPath, const char* szOutputTag, const plPlatformProfile* pAssetProfile) const override;
  virtual bool IsOutputUpToDate(
    const char* szDocumentPath, const char* szOutputTag, plUInt64 uiHash, const plAssetDocumentTypeDescriptor* pTypeDescriptor) override;

  static const char* const s_szShaderOutputTag;

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual void InternalCreateDocument(
    const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return false; }

private:
  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
