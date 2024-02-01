#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

struct plRmlUiResourceDescriptor;

class plRmlUiAssetDocument : public plSimpleAssetDocument<plRmlUiAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plRmlUiAssetDocument, plSimpleAssetDocument<plRmlUiAssetProperties>);

public:
  plRmlUiAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
