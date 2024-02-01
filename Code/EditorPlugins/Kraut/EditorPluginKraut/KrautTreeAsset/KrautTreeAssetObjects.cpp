#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetObjects.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plKrautAssetMaterial, plNoBase, 1, plRTTIDefaultAllocator<plKrautAssetMaterial>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new plReadOnlyAttribute()),
    PL_MEMBER_PROPERTY("Material", m_sMaterial)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plKrautTreeAssetProperties, 1, plRTTIDefaultAllocator<plKrautTreeAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("KrautFile", m_sKrautFile)->AddAttributes(new plFileBrowserAttribute("Select Kraut Tree file", "*.tree")),
    PL_MEMBER_PROPERTY("UniformScaling", m_fUniformScaling)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("LodDistanceScale", m_fLodDistanceScale)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("StaticColliderRadius", m_fStaticColliderRadius)->AddAttributes(new plDefaultValueAttribute(0.4f), new plClampValueAttribute(0.0f, 10.0f)),
    PL_MEMBER_PROPERTY("TreeStiffness", m_fTreeStiffness)->AddAttributes(new plDefaultValueAttribute(10.0f), new plClampValueAttribute(1.0f, 10000.0f)),
    PL_MEMBER_PROPERTY("Surface", m_sSurface)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Surface", plDependencyFlags::Package)),
    PL_ARRAY_MEMBER_PROPERTY("Materials", m_Materials)->AddAttributes(new plContainerAttribute(false, false, false)),
    PL_MEMBER_PROPERTY("DisplayRandomSeed", m_uiRandomSeedForDisplay),
    PL_ARRAY_MEMBER_PROPERTY("GoodRandomSeeds", m_GoodRandomSeeds),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plKrautTreeAssetProperties::plKrautTreeAssetProperties() = default;
plKrautTreeAssetProperties::~plKrautTreeAssetProperties() = default;
