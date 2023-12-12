#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>

class plGeometry;
class plChunkStreamWriter;
struct plJoltCookingMesh;

class plJoltCollisionMeshAssetDocument : public plSimpleAssetDocument<plJoltCollisionMeshAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltCollisionMeshAssetDocument, plSimpleAssetDocument<plJoltCollisionMeshAssetProperties>);

public:
  plJoltCollisionMeshAssetDocument(const char* szDocumentPath, bool bConvexMesh);

  static plStatus WriteToStream(plChunkStreamWriter& inout_stream, const plJoltCookingMesh& mesh, const plJoltCollisionMeshAssetProperties* pProp);

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus CreateMeshFromFile(plJoltCookingMesh& outMesh);
  plStatus CreateMeshFromGeom(plGeometry& geom, plJoltCookingMesh& outMesh);
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  bool m_bIsConvexMesh = false;

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////


class plJoltCollisionMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltCollisionMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plJoltCollisionMeshAssetDocumentGenerator();
  ~plJoltCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual plStatus Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plJoltCollisionMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
  virtual plStringView GetNameSuffix() const override { return "collision"; }
};

class plJoltConvexCollisionMeshAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plJoltConvexCollisionMeshAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plJoltConvexCollisionMeshAssetDocumentGenerator();
  ~plJoltConvexCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_modes) const override;
  virtual plStatus Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plJoltConvexCollisionMeshAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "JoltCollisionMeshes"; }
  virtual plStringView GetNameSuffix() const override { return "conv_collision"; }
};
