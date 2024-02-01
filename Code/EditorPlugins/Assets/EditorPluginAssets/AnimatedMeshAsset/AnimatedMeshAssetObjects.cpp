#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimatedMeshAssetProperties, 2, plRTTIDefaultAllocator<plAnimatedMeshAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new plFileBrowserAttribute("Select Mesh", plFileBrowserAttribute::MeshesWithAnimations)),
    PL_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    PL_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    PL_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ENUM_MEMBER_PROPERTY("NormalPrecision", plMeshNormalPrecision, m_NormalPrecision),
    PL_ENUM_MEMBER_PROPERTY("TexCoordPrecision", plMeshTexCoordPrecision, m_TexCoordPrecision),
    PL_ENUM_MEMBER_PROPERTY("BoneWeightPrecision", plMeshBoneWeigthPrecision, m_BoneWeightPrecision),
    PL_MEMBER_PROPERTY("NormalizeWeights", m_bNormalizeWeights)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new plContainerAttribute(false, true, true)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimatedMeshAssetProperties::plAnimatedMeshAssetProperties() = default;
plAnimatedMeshAssetProperties::~plAnimatedMeshAssetProperties() = default;
