#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>

class plTextureAssetProfileConfig;

struct plTextureChannelMode
{
  using StorageType = plUInt8;

  enum Enum
  {
    RGBA,
    RGB,
    Red,
    Green,
    Blue,
    Alpha,

    Default = RGBA
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plTextureChannelMode);

class plTextureAssetDocument : public plSimpleAssetDocument<plTextureAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plTextureAssetDocument, plSimpleAssetDocument<plTextureAssetProperties>);

public:
  plTextureAssetDocument(plStringView sDocumentPath);

  // for previewing purposes
  plEnum<plTextureChannelMode> m_ChannelMode;
  plInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level
  bool m_bIsRenderTarget = false;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override { return plStatus(PL_SUCCESS); }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus RunTexConv(const char* szTargetFile, const plAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const plTextureAssetProfileConfig* pAssetConfig);

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class plTextureAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plTextureAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plTextureAssetDocumentGenerator();
  ~plTextureAssetDocumentGenerator();

  enum class TextureType
  {
    Diffuse,
    Normal,
    Occlusion,
    Roughness,
    Metalness,
    ORM,
    Height,
    HDR,
    Linear,
  };

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plTextureAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Images"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments) override;

  static TextureType DetermineTextureType(plStringView sFile);
};
