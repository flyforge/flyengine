#pragma once

#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class plAnimatedMeshAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAnimatedMeshAssetProperties, plReflectedClass);

public:
  plAnimatedMeshAssetProperties();
  ~plAnimatedMeshAssetProperties();

  plString m_sMeshFile;
  plString m_sDefaultSkeleton;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTrangents = true;
  bool m_bNormalizeWeights = false;
  bool m_bImportMaterials = true;

  plEnum<plMeshNormalPrecision> m_NormalPrecision;
  plEnum<plMeshTexCoordPrecision> m_TexCoordPrecision;
  plEnum<plMeshBoneWeigthPrecision> m_BoneWeightPrecision;

  plHybridArray<plMaterialResourceSlot, 8> m_Slots;
};
