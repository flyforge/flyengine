#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Util/AssetUtils.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plMaterialResourceSlot, plNoBase, 1, plRTTIDefaultAllocator<plMaterialResourceSlot>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new plReadOnlyAttribute()),
    PL_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PL_MEMBER_PROPERTY("Highlight", m_bHighlight)->AddAttributes(new plTemporaryAttribute()),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on
