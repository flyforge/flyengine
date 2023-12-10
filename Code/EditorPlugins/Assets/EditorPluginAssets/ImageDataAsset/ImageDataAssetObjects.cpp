#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plImageDataAssetProperties, 1, plRTTIDefaultAllocator<plImageDataAssetProperties>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_sInputFile)->AddAttributes(new plFileBrowserAttribute("Select Image", plFileBrowserAttribute::ImagesLdrAndHdr))
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
