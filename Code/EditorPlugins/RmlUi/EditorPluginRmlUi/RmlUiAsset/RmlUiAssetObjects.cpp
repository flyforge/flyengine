#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRmlUiAssetProperties, 1, plRTTIDefaultAllocator<plRmlUiAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("RmlFile", m_sRmlFile)->AddAttributes(new plFileBrowserAttribute("Select Rml file", "*.rml")),
    PL_ENUM_MEMBER_PROPERTY("ScaleMode", plRmlUiScaleMode, m_ScaleMode),
    PL_MEMBER_PROPERTY("ReferenceResolution", m_ReferenceResolution)->AddAttributes(new plDefaultValueAttribute(plVec2U32(1920, 1080))),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRmlUiAssetProperties::plRmlUiAssetProperties() = default;
plRmlUiAssetProperties::~plRmlUiAssetProperties() = default;
