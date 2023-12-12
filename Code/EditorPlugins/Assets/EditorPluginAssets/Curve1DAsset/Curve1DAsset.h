#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

class plCurve1D;

class plCurve1DAssetDocument : public plSimpleAssetDocument<plCurveGroupData>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCurve1DAssetDocument, plSimpleAssetDocument<plCurveGroupData>);

public:
  plCurve1DAssetDocument(const char* szDocumentPath);
  ~plCurve1DAssetDocument();

  /// \brief Fills out the plCurve1D structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the curve, that must be called manually.
  void FillCurve(plUInt32 uiCurveIdx, plCurve1D& out_Result) const;

  plUInt32 GetCurveCount() const;

  void WriteResource(plStreamWriter& stream) const;

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
