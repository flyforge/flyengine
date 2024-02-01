#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class plMeshResourceDescriptor;
class plGeometry;
class plMaterialAssetDocument;

class plMeshAssetDocument : public plSimpleAssetDocument<plMeshAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plMeshAssetDocument, plSimpleAssetDocument<plMeshAssetProperties>);

public:
  plMeshAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  void CreateMeshFromGeom(plMeshAssetProperties* pProp, plMeshResourceDescriptor& desc);
  plTransformStatus CreateMeshFromFile(plMeshAssetProperties* pProp, plMeshResourceDescriptor& desc, bool bAllowMaterialImport);

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class plMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plMeshAssetDocumentGenerator();
  ~plMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments) override;
};
