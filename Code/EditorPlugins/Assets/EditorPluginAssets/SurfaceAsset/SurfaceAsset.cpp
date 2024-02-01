#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSurfaceAssetDocument, 2, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSurfaceAssetDocument::plSurfaceAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plSurfaceResourceDescriptor>(sDocumentPath, plAssetDocEngineConnection::None)
{
}

plTransformStatus plSurfaceAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
  const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  const plSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return plStatus(PL_SUCCESS);
}
