#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plCollectionAssetEntry : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plCollectionAssetEntry, plReflectedClass);

public:
  plString m_sLookupName;
  plString m_sRedirectionAsset;
};

class plCollectionAssetData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plCollectionAssetData, plReflectedClass);

public:
  plDynamicArray<plCollectionAssetEntry> m_Entries;
};

class plCollectionAssetDocument : public plSimpleAssetDocument<plCollectionAssetData>
{
  PL_ADD_DYNAMIC_REFLECTION(plCollectionAssetDocument, plSimpleAssetDocument<plCollectionAssetData>);

public:
  plCollectionAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};
