#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class plTextureAssetProfileConfig;

class plLUTAssetDocument : public plSimpleAssetDocument<plLUTAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLUTAssetDocument, plSimpleAssetDocument<plLUTAssetProperties>);

public:
  plLUTAssetDocument(const char* szDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override
  {
    return plStatus(PLASMA_SUCCESS);
  }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class plLUTAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLUTAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plLUTAssetDocumentGenerator();
  ~plLUTAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual plStatus Generate(
    plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plLUTAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "LUTs"; }
  virtual plStringView GetNameSuffix() const override { return "LUT"; }
};
