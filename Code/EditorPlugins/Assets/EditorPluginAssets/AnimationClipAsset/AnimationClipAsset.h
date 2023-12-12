#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

class plAnimationClipAssetDocument;
struct plPropertyMetaStateEvent;

struct plRootMotionSource
{
  using StorageType = plUInt8;

  enum Enum
  {
    None,
    Constant,
    // FromFeet,
    // AvgFromFeet,

    Default = None
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_NO_LINKAGE, plRootMotionSource);

class plAnimationClipAssetProperties : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimationClipAssetProperties, plReflectedClass);

public:
  plAnimationClipAssetProperties();
  ~plAnimationClipAssetProperties();

  plString m_sSourceFile;
  plString m_sAnimationClipToExtract;
  plDynamicArray<plString> m_AvailableClips;
  bool m_bAdditive = false;
  plUInt32 m_uiFirstFrame = 0;
  plUInt32 m_uiNumFrames = 0;
  plString m_sPreviewMesh;
  plEnum<plRootMotionSource> m_RootMotionMode;
  plVec3 m_vConstantRootMotion;

  plEventTrackData m_EventTrack;

  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);
};

//////////////////////////////////////////////////////////////////////////

class plAnimationClipAssetDocument : public plSimpleAssetDocument<plAnimationClipAssetProperties>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimationClipAssetDocument, plSimpleAssetDocument<plAnimationClipAssetProperties>);

public:
  plAnimationClipAssetDocument(const char* szDocumentPath);

  virtual void SetCommonAssetUiState(plCommonAssetUiState::Enum state, double value) override;
  virtual double GetCommonAssetUiState(plCommonAssetUiState::Enum state) const override;

  plUuid InsertEventTrackCpAt(plInt64 tickX, const char* szValue);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  // void ApplyCustomRootMotion(plAnimationClipResourceDescriptor& anim) const;
  // void ExtractRootMotionFromFeet(plAnimationClipResourceDescriptor& anim, const plSkeleton& skeleton) const;
  // void MakeRootMotionConstantAverage(plAnimationClipResourceDescriptor& anim) const;

private:
  float m_fSimulationSpeed = 1.0f;
};

//////////////////////////////////////////////////////////////////////////

class plAnimationClipAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAnimationClipAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plAnimationClipAssetDocumentGenerator();
  ~plAnimationClipAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sParentDirRelativePath, plHybridArray<plAssetDocumentGenerator::Info, 4>& out_Modes) const override;
  virtual plStatus Generate(plStringView sDataDirRelativePath, const plAssetDocumentGenerator::Info& info, plDocument*& out_pGeneratedDocument) override;
  virtual plStringView GetDocumentExtension() const override { return "plAnimationClipAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "AnimationClipGroup"; }
  virtual plStringView GetNameSuffix() const override { return "clip"; }
};
