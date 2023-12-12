#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

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

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_EDITORPLUGINSUBSTANCE_DLL, plSubstanceUsage);


struct plSubstanceGraphOutput
{
  bool m_bEnabled = true;
  bool m_bUseHighCompression = true;
  plEnum<plSubstanceUsage> m_Usage;
  plUInt8 m_uiNumChannels = 1;
  plString m_sName;
  plString m_sLabel;
  plUuid m_Uuid;

  bool operator==(const plSubstanceGraphOutput& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_bUseHighCompression == other.m_bUseHighCompression &&
           m_Usage == other.m_Usage &&
           m_uiNumChannels == other.m_uiNumChannels &&
           m_sName == other.m_sName &&
           m_sLabel == other.m_sLabel &&
           m_Uuid == other.m_Uuid;
  }
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_EDITORPLUGINSUBSTANCE_DLL, plSubstanceGraphOutput);


struct plSubstanceGraph
{
  bool m_bEnabled = true;

  plString m_sName;

  plHybridArray<plSubstanceGraphOutput, 8> m_Outputs;

  bool operator==(const plSubstanceGraph& other) const
  {
    return m_bEnabled == other.m_bEnabled &&
           m_sName == other.m_sName &&
           m_Outputs == other.m_Outputs;
  }
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_EDITORPLUGINSUBSTANCE_DLL, plSubstanceGraph);


class plSubstancePackageAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSubstancePackageAssetProperties, plReflectedClass);

public:
  plSubstancePackageAssetProperties() = default;

  plString m_sSubstancePackage;
  plString m_sOutputPattern;

  plHybridArray<plSubstanceGraph, 2> m_Graphs;
};


class plSubstancePackageAssetMetaData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSubstancePackageAssetMetaData, plReflectedClass);

public:
  plDynamicArray<plUuid> m_OutputUuids;
  plDynamicArray<plString> m_OutputNames;
};


class plSubstancePackageAssetDocument : public plSimpleAssetDocument<plSubstancePackageAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSubstancePackageAssetDocument, plSimpleAssetDocument<plSubstancePackageAssetProperties>);

public:
  plSubstancePackageAssetDocument(const char* szDocumentPath);
  ~plSubstancePackageAssetDocument();

private:
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override { return plStatus(PLASMA_SUCCESS); }
  virtual plTransformStatus InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  void OnPropertyChanged(const plDocumentObjectPropertyEvent& e);

private:
  plResult GetTempDir(plStringBuilder& out_sTempDir) const;
  void GenerateOutputName(const plSubstanceGraph& graph, const plSubstanceGraphOutput& graphOutput, plStringBuilder& out_sOutputName) const;
  plTransformStatus UpdateGraphOutputs(plStringView sAbsolutePath, bool bAllowPropertyModifications);
  plStatus RunTexConv(const char* szInputFile, const char* szTargetFile, const plAssetFileHeader& assetHeader, const plSubstanceGraphOutput& graphOutput, plStringView sThumbnailFile, const plTextureAssetProfileConfig* pAssetConfig);
};
