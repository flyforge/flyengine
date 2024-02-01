#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetObjects.h>

class plTextureAssetProfileConfig;

class plLUTAssetDocument : public plSimpleAssetDocument<plLUTAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plLUTAssetDocument, plSimpleAssetDocument<plLUTAssetProperties>);

public:
  plLUTAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override
  {
    return plStatus(PL_SUCCESS);
  }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////

class plLUTAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plLUTAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plLUTAssetDocumentGenerator();
  ~plLUTAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plLUTAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "LUTs"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments) override;
};
