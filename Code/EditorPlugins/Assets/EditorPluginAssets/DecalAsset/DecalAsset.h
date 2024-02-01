#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plAssetFileHeader;
struct plPropertyMetaStateEvent;

struct plDecalMode
{
  using StorageType = plInt8;

  enum Enum
  {
    BaseColor,
    BaseColorNormal,
    BaseColorORM,
    BaseColorNormalORM,
    BaseColorEmissive,

    Default = BaseColor
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_NO_LINKAGE, plDecalMode);

class plDecalAssetProperties : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plDecalAssetProperties, plReflectedClass);

public:
  plDecalAssetProperties();

  static void PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e);

  plEnum<plDecalMode> m_Mode;
  bool m_bBlendModeColorize = false;

  plString m_sAlphaMask;
  plString m_sBaseColor;
  plString m_sNormal;
  plString m_sORM;
  plString m_sEmissive;

  bool NeedsBaseColor() const { return true; }
  bool NeedsNormal() const { return m_Mode == plDecalMode::BaseColorNormal || m_Mode == plDecalMode::BaseColorNormalORM; }
  bool NeedsORM() const { return m_Mode == plDecalMode::BaseColorORM || m_Mode == plDecalMode::BaseColorNormalORM; }
  bool NeedsEmissive() const { return m_Mode == plDecalMode::BaseColorEmissive; }
};


class plDecalAssetDocument : public plSimpleAssetDocument<plDecalAssetProperties>
{
  PL_ADD_DYNAMIC_REFLECTION(plDecalAssetDocument, plSimpleAssetDocument<plDecalAssetProperties>);

public:
  plDecalAssetDocument(plStringView sDocumentPath);

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;

  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};

//////////////////////////////////////////////////////////////////////////

class plDecalAssetDocumentGenerator : public plAssetDocumentGenerator
{
  PL_ADD_DYNAMIC_REFLECTION(plDecalAssetDocumentGenerator, plAssetDocumentGenerator);

public:
  plDecalAssetDocumentGenerator();
  ~plDecalAssetDocumentGenerator();

  virtual void GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual plStringView GetDocumentExtension() const override { return "plDecalAsset"; }
  virtual plStringView GetGeneratorGroup() const override { return "Images"; }
  virtual plStatus Generate(plStringView sInputFileAbs, plStringView sMode, plDocument*& out_pGeneratedDocument) override;
};
