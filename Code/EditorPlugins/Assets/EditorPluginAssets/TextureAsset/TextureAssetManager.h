#pragma once

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

class PLASMA_EDITORPLUGINASSETS_DLL plTextureAssetProfileConfig : public plProfileConfigData
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureAssetProfileConfig, plProfileConfigData);

public:
  plUInt16 m_uiMaxResolution = 1024 * 16;
};

class plTextureAssetDocumentManager : public plAssetDocumentManager
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureAssetDocumentManager, plAssetDocumentManager);

public:
  plTextureAssetDocumentManager();
  ~plTextureAssetDocumentManager();

  virtual OutputReliability GetAssetTypeOutputReliability() const override { return plAssetDocumentManager::OutputReliability::Perfect; }

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual plUInt64 ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const override;

  virtual void InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual plString GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDescriptor, plStringView sDataDirectory, plStringView sDocumentPath, plStringView sOutputTag, const plPlatformProfile* pAssetProfile) const override;

private:
  plAssetDocumentTypeDescriptor m_DocTypeDesc;
  plAssetDocumentTypeDescriptor m_DocTypeDesc2;
};
