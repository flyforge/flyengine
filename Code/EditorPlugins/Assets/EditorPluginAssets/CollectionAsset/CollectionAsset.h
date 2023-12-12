#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plCollectionAssetEntry : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCollectionAssetEntry, plReflectedClass);

public:
  plString m_sLookupName;
  plString m_sRedirectionAsset;
};

class plCollectionAssetData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCollectionAssetData, plReflectedClass);

public:
  plDynamicArray<plCollectionAssetEntry> m_Entries;
};

class plCollectionAssetDocument : public plSimpleAssetDocument<plCollectionAssetData>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCollectionAssetDocument, plSimpleAssetDocument<plCollectionAssetData>);

public:
  plCollectionAssetDocument(const char* szDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;

  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};
