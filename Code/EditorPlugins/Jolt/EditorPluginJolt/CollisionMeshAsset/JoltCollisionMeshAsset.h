#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>

class plGeometry;
class plChunkStreamWriter;
struct plJoltCookingMesh;

class plJoltCollisionMeshAssetDocument : public plSimpleAssetDocument<plJoltCollisionMeshAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plJoltCollisionMeshAssetDocument, plSimpleAssetDocument<plJoltCollisionMeshAssetProperties>);

public:
  plJoltCollisionMeshAssetDocument(plStringView sDocumentPath, bool bConvexMesh);

  static plStatus WriteToStream(plChunkStreamWriter& inout_stream, const plJoltCookingMesh& mesh, const plJoltCollisionMeshAssetProperties* pProp);

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus CreateMeshFromFile(plJoltCookingMesh& outMesh);
  plStatus CreateMeshFromGeom(plGeometry& geom, plJoltCookingMesh& outMesh);
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  bool m_bIsConvexMesh = false;

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////


class plJoltCollisionMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plJoltCollisionMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plJoltCollisionMeshAssetDocumentGenerator();
  ~plJoltCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plJoltCollisionMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};

class plJoltConvexCollisionMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plJoltConvexCollisionMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plJoltConvexCollisionMeshAssetDocumentGenerator();
  ~plJoltConvexCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plJoltConvexCollisionMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};
