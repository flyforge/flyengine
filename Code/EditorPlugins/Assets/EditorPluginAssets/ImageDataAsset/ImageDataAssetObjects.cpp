#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plImageDataAssetProperties, 1, plRTTIDefaultAllocator<plImageDataAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Input", m_sInputFile)->AddAttributes(new plFileBrowserAttribute("Select Image", plFileBrowserAttribute::ImagesLdrAndHdr))
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
