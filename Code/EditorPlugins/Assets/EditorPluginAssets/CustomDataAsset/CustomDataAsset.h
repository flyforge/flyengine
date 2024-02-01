#pragma once

#include <Core/Utils/CustomData.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plCustomDataAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plCustomDataAssetProperties, plReflectedClass);

public:
  plCustomData* m_pType = nullptr;
};


class plCustomDataAssetDocument : public plSimpleAssetDocument<plCustomDataAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plCustomDataAssetDocument, plSimpleAssetDocument<plCustomDataAssetProperties>);

public:
  plCustomDataAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};
