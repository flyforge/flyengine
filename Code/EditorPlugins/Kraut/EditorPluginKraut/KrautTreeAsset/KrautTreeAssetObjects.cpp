#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plKrautAssetMaterial, plNoBase, 1, plRTTIDefaultAllocator<plKrautAssetMaterial>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new plReadOnlyAttribute()),
    PLASMA_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetProperties, 1, plRTTIDefaultAllocator<plKrautTreeAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("KrautFile", m_sKrautFile)->AddAttributes(new plFileBrowserAttribute("Select Kraut Tree file", "*.tree")),
    PLASMA_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new plDefaultValueAttribute(0.4f), new plClampValueAttribute(0.0f, 10.0f)),
    PLASMA_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(1.0f, 10000.0f)),
    PLASMA_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PLASMA_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new plContainerAttribute(false, false, false)),
    PLASMA_MEMBER_PROPERTY("DisplayRandomSeed", m_uiRandomSeedForDisplay),
    PLASMA_ARRAY_MEMBER_PROPERTY("GoodRandomSeeds", m_GoodRandomSeeds),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plKrautTreeAssetProperties::plKrautTreeAssetProperties() = default;
plKrautTreeAssetProperties::~plKrautTreeAssetProperties() = default;
