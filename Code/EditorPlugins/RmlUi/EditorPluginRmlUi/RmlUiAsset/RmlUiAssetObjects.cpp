#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiAssetProperties, 1, plRTTIDefaultAllocator<plRmlUiAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RmlFile", m_sRmlFile)->AddAttributes(new plFileBrowserAttribute("Select Rml file", "*.rml")),
    PLASMA_ENUM_MEMBER_PROPERTY("ScaleMode", plRmlUiScaleMode, m_ScaleMode),
    PLASMA_MEMBER_PROPERTY("ReferenceResolution", m_ReferenceResolution)->AddAttributes(new plDefaultValueAttribute(plVec2U32(1920, 1080))),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRmlUiAssetProperties::plRmlUiAssetProperties() = default;
plRmlUiAssetProperties::~plRmlUiAssetProperties() = default;
