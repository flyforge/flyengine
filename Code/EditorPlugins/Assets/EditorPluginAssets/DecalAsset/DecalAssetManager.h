#pragma once

#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/Types/Status.h>

class plDecalAssetDocumentManager : public plAssetDocumentManager
{
  PL_ADD_DYNAMIC_REFLECTION(plDecalAssetDocumentManager, plAssetDocumentManager);

public:
  plDecalAssetDocumentManager();
  ~plDecalAssetDocumentManager();

  virtual void AddEntriesToAssetTable(
    plStringView sDataDirectory, const plPlatformProfile* pAssetProfile, plDelegate<void(plStringView sGuid, plStringView sPath, plStringView sType)> addEntry) const override;
  virtual plString GetAssetTableEntry(
    const plSubAsset* pSubAsset, plStringView sDataDirectory, const plPlatformProfile* pAssetProfile) const override;

  /// \brief There is only a single decal texture per project. This function creates it, in case any decal asset was modified.
  plStatus GenerateDecalTexture(const plPlatformProfile* pAssetProfile);
  plString GetDecalTexturePath(const plPlatformProfile* pAssetProfile) const;

private:
  void OnDocumentManagerEvent(const plDocumentManager::Event& e);
  bool IsDecalTextureUpToDate(const char* szDecalFile, plUInt64 uiAssetHash) const;
  plStatus RunTexConv(const char* szTargetFile, const char* szInputFile, const plAssetFileHeader& AssetHeader);

  virtual void InternalCreateDocument(
    plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext) override;
  virtual void InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const override;

  virtual bool GeneratesProfileSpecificAssets() const override { return true; }

  virtual plUInt64 ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const override;

  plAssetDocumentTypeDescriptor m_DocTypeDesc;
};
