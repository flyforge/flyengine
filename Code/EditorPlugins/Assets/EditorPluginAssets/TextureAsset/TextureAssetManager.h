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

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);

  virtual plUInt64 ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const override;

  virtual void InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual plString GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDescriptor, const char* szDataDirectory, const char* szDocumentPath, const char* szOutputTag, const plPlatformProfile* pAssetProfile) const override;

private:
  plAssetDocumentTypeDescriptor m_DocTypeDesc;
  plAssetDocumentTypeDescriptor m_DocTypeDesc2;
};
