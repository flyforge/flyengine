#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class plTextureAssetProfileConfig;

class plLUTAssetDocument : public plSimpleAssetDocument<plLUTAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLUTAssetDocument, plSimpleAssetDocument<plLUTAssetProperties>);

public:
  plLUTAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override
  {
    return plStatus(PLASMA_SUCCESS);
  }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class plLUTAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plLUTAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plLUTAssetDocumentGenerator();
  ~plLUTAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plLUTAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "LUTs"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};
