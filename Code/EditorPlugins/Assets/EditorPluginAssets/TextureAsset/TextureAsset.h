#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>

class plTextureAssetProfileConfig;

struct plTextureChannelMode
{
  typedef plUInt8 StorageType;

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
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plTextureChannelMode);

class plTextureAssetDocument : public plSimpleAssetDocument<plTextureAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureAssetDocument, plSimpleAssetDocument<plTextureAssetProperties>);

public:
  plTextureAssetDocument(const char* szDocumentPath);

  // for previewing purposes
  plEnum<plTextureChannelMode> m_ChannelMode;
  plInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level
  bool m_bIsRenderTarget = false;

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override { return plStatus(PLASMA_SUCCESS); }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  plStatus RunTexConv(const char* szTargetFile, const plAssetFileHeader& AssetHeader, bool bUpdateThumbnail, const plTextureAssetProfileConfig* pAssetConfig);

  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class plTextureAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plTextureAssetDocumentGenerator();
  ~plTextureAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual plStatus Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plTextureAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Images"; }
  virtual plStringView GetNameSuffix() const override { return "texture"; }
};
