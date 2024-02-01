#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

struct plKrautTreeResourceDescriptor;
struct plKrautGeneratorResourceDescriptor;

namespace plModelImporter2
{
  enum class TextureSemantic : plInt8;
}

class plKrautTreeAssetDocument : public plSimpleAssetDocument<plKrautTreeAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetDocument, plSimpleAssetDocument<plKrautTreeAssetProperties>);

public:
  plKrautTreeAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  void SyncBackAssetProperties(plKrautTreeAssetProperties*& pProp, const plKrautGeneratorResourceDescriptor& desc);

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////


class plKrautTreeAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plKrautTreeAssetDocumentGenerator();
  ~plKrautTreeAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plKrautTreeAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "KrautTrees"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};
