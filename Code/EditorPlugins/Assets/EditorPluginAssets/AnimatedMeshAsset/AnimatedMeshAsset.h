#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

class plMeshResourceDescriptor;
class plMaterialAssetDocument;

class plAnimatedMeshAssetDocument : public plSimpleAssetDocument<plAnimatedMeshAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimatedMeshAssetDocument, plSimpleAssetDocument<plAnimatedMeshAssetProperties>);

public:
  plAnimatedMeshAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus CreateMeshFromFile(plAnimatedMeshAssetProperties* pProp, plMeshResourceDescriptor& desc);


  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class plAnimatedMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimatedMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plAnimatedMeshAssetDocumentGenerator();
  ~plAnimatedMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual plStatus Generate(
    plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plAnimatedMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual plStringView GetNameSuffix() const override { return "animated_mesh"; }
};
