#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetObjects.h>

struct plTextureCubeChannelMode
{
  typedef plUInt8 StorageType;

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
  plTextureCubeAssetDocument(const char* szDocumentPath);

  // for previewing purposes
  plEnum<plTextureCubeChannelMode> m_ChannelMode;
  plInt32 m_iTextureLod; // -1 == regular sampling, >= 0 == sample that level

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override
  {
    return plStatus(PLASMA_SUCCESS);
  }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

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

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual plStatus Generate(
    plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plTextureCubeAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Images"; }
  virtual plStringView GetNameSuffix() const override { return "texture_cube"; }
};
