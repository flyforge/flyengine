#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetObjects.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimatedMeshAssetProperties, 2, plRTTIDefaultAllocator<plAnimatedMeshAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MeshFile", m_sMeshFile)->AddAttributes(new plFileBrowserAttribute("Select Mesh", plFileBrowserAttribute::MeshesWithAnimations)),
    PLASMA_MEMBER_PROPERTY("DefaultSkeleton", m_sDefaultSkeleton)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Skeleton")),
    PLASMA_MEMBER_PROPERTY("RecalculateNormals", m_bRecalculateNormals),
    PLASMA_MEMBER_PROPERTY("RecalculateTangents", m_bRecalculateTrangents)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ENUM_MEMBER_PROPERTY("NormalPrecision", plMeshNormalPrecision, m_NormalPrecision),
    PLASMA_ENUM_MEMBER_PROPERTY("TexCoordPrecision", plMeshTexCoordPrecision, m_TexCoordPrecision),
    PLASMA_ENUM_MEMBER_PROPERTY("BoneWeightPrecision", plMeshBoneWeigthPrecision, m_BoneWeightPrecision),
    PLASMA_MEMBER_PROPERTY("ImportMaterials", m_bImportMaterials)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ARRAY_MEMBER_PROPERTY("Materials", m_Slots)->AddAttributes(new plContainerAttribute(false, true, true)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimatedMeshAssetProperties::plAnimatedMeshAssetProperties() = default;
plAnimatedMeshAssetProperties::~plAnimatedMeshAssetProperties() = default;
