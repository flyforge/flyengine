#pragma once

#include <Core/CustomData/CustomData.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plCustomDataAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCustomDataAssetProperties, plReflectedClass);

public:
  plCustomData* m_pType = nullptr;
};


class plCustomDataAssetDocument : public plSimpleAssetDocument<plCustomDataAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCustomDataAssetDocument, plSimpleAssetDocument<plCustomDataAssetProperties>);

public:
  plCustomDataAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};