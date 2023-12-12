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
  PLASMA_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetDocument, plSimpleAssetDocument<plKrautTreeAssetProperties>);

public:
  plKrautTreeAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  void SyncBackAssetProperties(plKrautTreeAssetProperties*& pProp, const plKrautGeneratorResourceDescriptor& desc);

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////


class plKrautTreeAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plKrautTreeAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plKrautTreeAssetDocumentGenerator();
  ~plKrautTreeAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual plStatus Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plKrautTreeAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "KrautTrees"; }
  virtual plStringView GetNameSuffix() const override { return "kraut"; }
};
