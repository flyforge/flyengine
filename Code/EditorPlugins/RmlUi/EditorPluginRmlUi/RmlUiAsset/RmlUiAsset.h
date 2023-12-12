#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

struct plRmlUiResourceDescriptor;

class plRmlUiAssetDocument : public plSimpleAssetDocument<plRmlUiAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRmlUiAssetDocument, plSimpleAssetDocument<plRmlUiAssetProperties>);

public:
  plRmlUiAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class plRmlUiAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRmlUiAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plRmlUiAssetDocumentGenerator();
  ~plRmlUiAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual plStatus Generate(
    plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plRmlUiAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "RmlUis"; }
  virtual plStringView GetNameSuffix() const override { return "RmlUI"; }
};
