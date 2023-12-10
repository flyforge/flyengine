#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>

struct plTextureCubeChannelMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    RGB,
    Red,
    Green,
    Blue,
    Alpha,

    Default = RGB
  };
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTextureCubeChannelMode);

class plTextureCubeAssetDocument : public plSimpleAssetDocument<plTextureCubeAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureCubeAssetDocument, plSimpleAssetDocument<plTextureCubeAssetProperties>);

public:
  plTextureCubeAssetDocument(plStringView sDocumentPath);

  // for previewing purposes
  plEnum<plTextureCubeChannelMode> m_ChannelMode;
  plInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override
  {
    return plStatus(PLASMA_SUCCESS);
  }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus RunTexConv(const char* szTargetFile, const plAssetFileHeader& AssetHeader, bool bUpdateThumbnail);

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class plTextureCubeAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureCubeAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plTextureCubeAssetDocumentGenerator();
  ~plTextureCubeAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plTextureCubeAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Images"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};
