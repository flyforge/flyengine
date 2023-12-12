#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plSurfaceAssetDocument : public plSimpleAssetDocument<plSurfaceResourceDescriptor>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSurfaceAssetDocument, plSimpleAssetDocument<plSurfaceResourceDescriptor>);

public:
  plSurfaceAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};
