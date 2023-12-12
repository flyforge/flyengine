#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class plMeshResourceDescriptor;
class plGeometry;
class plMaterialAssetDocument;

class plMeshAssetDocument : public plSimpleAssetDocument<plMeshAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshAssetDocument, plSimpleAssetDocument<plMeshAssetProperties>);

public:
  plMeshAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  void CreateMeshFromGeom(plMeshAssetProperties* pProp, plMeshResourceDescriptor& desc);
  plTransformStatus CreateMeshFromFile(plMeshAssetProperties* pProp, plMeshResourceDescriptor& desc, bool bAllowMaterialImport);

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class plMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plMeshAssetDocumentGenerator();
  ~plMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual plStatus Generate(
    plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual plStringView GetNameSuffix() const override { return "mesh"; }
};
