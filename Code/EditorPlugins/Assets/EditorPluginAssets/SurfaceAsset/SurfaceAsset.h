#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plSurfaceAssetDocument : public plSimpleAssetDocument<plSurfaceResourceDescriptor>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSurfaceAssetDocument, plSimpleAssetDocument<plSurfaceResourceDescriptor>);

public:
  plSurfaceAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};
