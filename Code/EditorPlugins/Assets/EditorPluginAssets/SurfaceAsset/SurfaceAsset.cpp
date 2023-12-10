#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSurfaceAssetDocument, 2, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plSurfaceAssetDocument::plSurfaceAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plSurfaceResourceDescriptor>(sDocumentPath, plAssetDocEngineConnection::None)
{
}

plTransformStatus plSurfaceAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
  const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  const plSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return plStatus(PLASMA_SUCCESS);
}
