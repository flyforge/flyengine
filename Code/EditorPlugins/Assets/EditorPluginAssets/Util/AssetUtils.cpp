#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Util/AssetUtils.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plMaterialResourceSlot, plNoBase, 1, plRTTIDefaultAllocator<plMaterialResourceSlot>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new plReadOnlyAttribute()),
    PLASMA_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Material")),
    PLASMA_MEMBER_PROPERTY("Highlight", m_bHighlight)->AddAttributes(new plTemporaryAttribute()),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on
