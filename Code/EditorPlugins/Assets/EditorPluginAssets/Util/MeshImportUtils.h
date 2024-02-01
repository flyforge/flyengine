#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class plMeshResourceDescriptor;

namespace plModelImporter2
{
  class Importer;
  enum class TextureSemantic : plInt8;
} // namespace plModelImporter2

namespace plMeshImportUtils
{
  PL_EDITORPLUGINASSETS_DLL plString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, const char* szTexturePath, plModelImporter2::TextureSemantic hint, bool bTextureClamp);

  // PL_EDITORPLUGINASSETS_DLL void ImportMaterial(plMaterialAssetDocument* materialDocument, const plModelImporter::Material* material, const char* szImportSourceFolder, const char* szImportTargetFolder);

  // PL_EDITORPLUGINASSETS_DLL void ImportMeshMaterials(const plModelImporter::Scene& scene, const plModelImporter::Mesh& mesh, plHybridArray<plMaterialResourceSlot, 8>& inout_MaterialSlots, const char* szImportSourceFolder, const char* szImportTargetFolder);

  // PL_EDITORPLUGINASSETS_DLL void ImportMeshAssetMaterials(const char* szAssetDocument, const char* szMeshFile, bool bUseSubFolderForImportedMaterials, const plModelImporter::Scene& scene, const plModelImporter::Mesh& mesh, plHybridArray<plMaterialResourceSlot, 8>& inout_MaterialSlots);

  // PL_EDITORPLUGINASSETS_DLL const plString GetResourceSlotProperty(const plHybridArray<plMaterialResourceSlot, 8>& materialSlots, plUInt32 uiSlot);

  // PL_EDITORPLUGINASSETS_DLL void AddMeshToDescriptor(plMeshResourceDescriptor& meshDescriptor, const plModelImporter::Scene& scene, const plModelImporter::Mesh& mesh, const plHybridArray<plMaterialResourceSlot, 8>& materialSlots);

  // PL_EDITORPLUGINASSETS_DLL void UpdateMaterialSlots(plStringView sDocumentPath, const plModelImporter::Scene& scene, const plModelImporter::Mesh& mesh, bool bImportMaterials, bool bUseSubFolderForImportedMaterials, const char* szMeshFile, plHybridArray<plMaterialResourceSlot, 8>& inout_MaterialSlots);

  // PL_EDITORPLUGINASSETS_DLL void PrepareMeshForImport(plModelImporter::Mesh& mesh, bool bRecalculateNormals, plProgressRange& range);

  // PL_EDITORPLUGINASSETS_DLL plStatus GenerateMeshBuffer(const plModelImporter::Mesh& mesh, plMeshResourceDescriptor& meshDescriptor, const plMat3& mTransformation, bool bInvertNormals, plMeshNormalPrecision::Enum normalPrecision, plMeshTexCoordPrecision::Enum texCoordPrecision, bool bSkinnedMesh);

  // PL_EDITORPLUGINASSETS_DLL plStatus TryImportMesh(plSharedPtr<plModelImporter::Scene>& out_pScene, plModelImporter::Mesh*& out_pMesh, const char* szMeshFile, const char* szSubMeshName, const plMat3& mMeshTransform, bool bRecalculateNormals, bool bInvertNormals, plMeshNormalPrecision::Enum normalPrecision, plMeshTexCoordPrecision::Enum texCoordPrecision, plProgressRange& range, plMeshResourceDescriptor& meshDescriptor, bool bSkinnedMesh);

  PL_EDITORPLUGINASSETS_DLL void SetMeshAssetMaterialSlots(plHybridArray<plMaterialResourceSlot, 8>& inout_materialSlots, const plModelImporter2::Importer* pImporter);
  PL_EDITORPLUGINASSETS_DLL void CopyMeshAssetMaterialSlotToResource(plMeshResourceDescriptor& ref_desc, const plHybridArray<plMaterialResourceSlot, 8>& materialSlots);
  PL_EDITORPLUGINASSETS_DLL void ImportMeshAssetMaterials(plHybridArray<plMaterialResourceSlot, 8>& inout_materialSlots, plStringView sDocumentDirectory, const plModelImporter2::Importer* pImporter);
} // namespace plMeshImportUtils
