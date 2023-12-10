#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

struct plImageDataAssetEvent
{
  enum class Type
  {
    Transformed,
  };

  Type m_Type = Type::Transformed;
};

class plImageDataAssetDocument : public plSimpleAssetDocument<plImageDataAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plImageDataAssetDocument, plSimpleAssetDocument<plImageDataAssetProperties>);

public:
  plImageDataAssetDocument(plStringView sDocumentPath);

  const plEvent<const plImageDataAssetEvent&>& Events() const { return m_Events; }

protected:
  plEvent<const plImageDataAssetEvent&> m_Events;

  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override { return plStatus(PLASMA_SUCCESS); }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus RunTexConv(const char* szTargetFile, const plAssetFileHeader& AssetHeader, bool bUpdateThumbnail);
};
