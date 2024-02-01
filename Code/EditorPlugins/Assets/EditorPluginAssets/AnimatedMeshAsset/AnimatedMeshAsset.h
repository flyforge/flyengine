#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

class plMeshResourceDescriptor;
class plMaterialAssetDocument;

class plAnimatedMeshAssetDocument : public plSimpleAssetDocument<plAnimatedMeshAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimatedMeshAssetDocument, plSimpleAssetDocument<plAnimatedMeshAssetProperties>);

public:
  plAnimatedMeshAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus CreateMeshFromFile(plAnimatedMeshAssetProperties* pProp, plMeshResourceDescriptor& desc);


  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class plAnimatedMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimatedMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plAnimatedMeshAssetDocumentGenerator();
  ~plAnimatedMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plAnimatedMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};
