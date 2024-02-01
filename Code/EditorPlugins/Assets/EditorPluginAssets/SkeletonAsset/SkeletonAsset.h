#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/AnimationSystem/EditableSkeleton.h>

//////////////////////////////////////////////////////////////////////////

struct plPropertyMetaStateEvent;
class plSkeletonAssetDocument;

struct plSkeletonAssetEvent
{
  enum Type
  {
    RenderStateChanged,
    Transformed,
  };

  plSkeletonAssetDocument* m_pDocument = nullptr;
  Type m_Type;
};

class plSkeletonAssetDocument : public plSimpleAssetDocument<plEditableSkeleton>
{
  PL_ADD_DYNAMIC_REFLECTION(plSkeletonAssetDocument, plSimpleAssetDocument<plEditableSkeleton>);

public:
  plSkeletonAssetDocument(plStringView sDocumentPath);
  ~plSkeletonAssetDocument();

  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  plStatus WriteResource(plStreamWriter& inout_stream, const plEditableSkeleton& skeleton) const;

  bool m_bIsTransforming = false;

  virtual plManipulatorSearchStrategy GetManipulatorSearchStrategy() const override
  {
    return plManipulatorSearchStrategy::SelectedObject;
  }

  const plEvent<const plSkeletonAssetEvent&>& Events() const { return m_Events; }

  void SetRenderBones(bool bEnable);
  bool GetRenderBones() const { return m_bRenderBones; }

  void SetRenderColliders(bool bEnable);
  bool GetRenderColliders() const { return m_bRenderColliders; }

  void SetRenderJoints(bool bEnable);
  bool GetRenderJoints() const { return m_bRenderJoints; }

  void SetRenderSwingLimits(bool bEnable);
  bool GetRenderSwingLimits() const { return m_bRenderSwingLimits; }

  void SetRenderTwistLimits(bool bEnable);
  bool GetRenderTwistLimits() const { return m_bRenderTwistLimits; }

  void SetRenderPreviewMesh(bool bEnable);
  bool GetRenderPreviewMesh() const { return m_bRenderPreviewMesh; }

protected:
  virtual void UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const override;
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  const plEditableSkeleton* MergeWithNewSkeleton(plEditableSkeleton& newSkeleton);

  plEvent<const plSkeletonAssetEvent&> m_Events;
  bool m_bRenderBones = true;
  bool m_bRenderColliders = true;
  bool m_bRenderJoints = false; // currently not exposed
  bool m_bRenderSwingLimits = true;
  bool m_bRenderTwistLimits = true;
  bool m_bRenderPreviewMesh = true;
};

//////////////////////////////////////////////////////////////////////////

class plSkeletonAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plSkeletonAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plSkeletonAssetDocumentGenerator();
  ~plSkeletonAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plSkeletonAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "AnimationSkeletonGroup"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};
