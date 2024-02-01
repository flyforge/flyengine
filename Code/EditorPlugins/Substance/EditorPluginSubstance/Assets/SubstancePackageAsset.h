#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Texture/TexConv/TexConvEnums.h>

struct plSubstanceUsage
{
  using StorageType = plUInt8;

  enum Enum
  {
    Unknown,

    BaseColor,
    Emissive,
    Height,
    Metallic,
    Mask,
    Normal,
    Occlusion,
    Opacity,
    Roughness,

    Count,

    Default = Unknown
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORPLUGINSUBSTANCE_DLL, plSubstanceUsage);


struct plSubstanceGraphOutput
{
  bool m_bEnabled = true;
  plEnum<plTexConvCompressionMode> m_CompressionMode = plTexConvCompressionMode::High;
  plEnum<plSubstanceUsage> m_Usage;
  plUInt8 m_uiNumChannels = 1;
  bool m_bPreserveAlphaCoverage = false;
  plString m_sName;
  plString m_sLabel;
  plUuid m_Uuid;

  bool operator==(const plSubstanceGraphOutput& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_CompressionMode == other.m_CompressionMode &&
           m_Usage == other.m_Usage &&
           m_uiNumChannels == other.m_uiNumChannels &&
           m_bPreserveAlphaCoverage == other.m_bPreserveAlphaCoverage &&
           m_sName == other.m_sName &&
           m_sLabel == other.m_sLabel &&
           m_Uuid == other.m_Uuid;
  }
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORPLUGINSUBSTANCE_DLL, plSubstanceGraphOutput);


struct plSubstanceGraph
{
  bool m_bEnabled = true;

  plString m_sName;

  plUInt8 m_uiOutputWidth = 0; ///< In base 2, e.g. 8 = 2^8 = 256
  plUInt8 m_uiOutputHeight = 0; ///< In base 2

  plHybridArray<plSubstanceGraphOutput, 8> m_Outputs;

  bool operator==(const plSubstanceGraph& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_sName == other.m_sName &&
           m_uiOutputWidth == other.m_uiOutputWidth &&
           m_uiOutputHeight == other.m_uiOutputHeight &&
           m_Outputs == other.m_Outputs;
  }
};

PL_DECLARE_REFLECTABLE_TYPE(PL_EDITORPLUGINSUBSTANCE_DLL, plSubstanceGraph);


class plSubstancePackageAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plSubstancePackageAssetProperties, plReflectedClass);

public:
  plSubstancePackageAssetProperties() = default;

  plString m_sSubstancePackage;
  plString m_sOutputPattern;

  plHybridArray<plSubstanceGraph, 2> m_Graphs;
};


class plSubstancePackageAssetMetaData : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plSubstancePackageAssetMetaData, plReflectedClass);

public:
  plDynamicArray<plUuid> m_OutputUuids;
  plDynamicArray<plString> m_OutputNames;
};

class plTextureAssetProfileConfig;

class plSubstancePackageAssetDocument : public plSimpleAssetDocument<plSubstancePackageAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plSubstancePackageAssetDocument, plSimpleAssetDocument<plSubstancePackageAssetProperties>);

public:
  plSubstancePackageAssetDocument(plStringView sDocumentPath);
  ~plSubstancePackageAssetDocument();

private:
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override { return plStatus(PL_SUCCESS); }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  void OnPropertyChanged(const plDocumentObjectPropertyEvent& e);

private:
  plResult GetTempDir(plStringBuilder& out_sTempDir) const;
  void GenerateOutputName(const plSubstanceGraph& graph, const plSubstanceGraphOutput& graphOutput, plStringBuilder& out_sOutputName) const;
  plTransformStatus UpdateGraphOutputs(plStringView sAbsolutePath, bool bAllowPropertyModifications);
  plStatus RunTexConv(const char* szInputFile, const char* szTargetFile, const plAssetFileHeader& assetHeader, const plSubstanceGraphOutput& graphOutput, plStringView sThumbnailFile, const plTextureAssetProfileConfig* pAssetConfig);
};
