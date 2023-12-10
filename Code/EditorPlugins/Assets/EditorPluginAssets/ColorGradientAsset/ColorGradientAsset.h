#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>

class plColorGradient;

class plColorControlPoint : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plColorControlPoint, plReflectedClass);

public:
  plTime GetTickAsTime() const { return plTime::MakeFromSeconds(m_iTick / 4800.0); }
  void SetTickFromTime(plTime time, plInt64 iFps);

  // double m_fPositionX;
  plInt64 m_iTick; // 4800 ticks per second
  plUInt8 m_Red;
  plUInt8 m_Green;
  plUInt8 m_Blue;
};

class plAlphaControlPoint : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plAlphaControlPoint, plReflectedClass);

public:
  plTime GetTickAsTime() const { return plTime::MakeFromSeconds(m_iTick / 4800.0); }
  void SetTickFromTime(plTime time, plInt64 iFps);

  // double m_fPositionX;
  plInt64 m_iTick; // 4800 ticks per second
  plUInt8 m_Alpha;
};

class plIntensityControlPoint : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plIntensityControlPoint, plReflectedClass);

public:
  plTime GetTickAsTime() const { return plTime::MakeFromSeconds(m_iTick / 4800.0); }
  void SetTickFromTime(plTime time, plInt64 iFps);

  // double m_fPositionX;
  plInt64 m_iTick; // 4800 ticks per second
  float m_fIntensity;
};

class plColorGradientAssetData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plColorGradientAssetData, plReflectedClass);

public:
  plDynamicArray<plColorControlPoint> m_ColorCPs;
  plDynamicArray<plAlphaControlPoint> m_AlphaCPs;
  plDynamicArray<plIntensityControlPoint> m_IntensityCPs;

  static plInt64 TickFromTime(plTime time);

  /// \brief Fills out the plColorGradient structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the color gradient, that must be called manually.
  void FillGradientData(plColorGradient& out_result) const;
  plColor Evaluate(plInt64 iTick) const;
};

class plColorGradientAssetDocument : public plSimpleAssetDocument<plColorGradientAssetData>
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plColorGradientAssetDocument, plSimpleAssetDocument<plColorGradientAssetData>);

public:
  plColorGradientAssetDocument(plStringView sDocumentPath);

  void WriteResource(plStreamWriter& inout_stream) const;

protected:
  virtual plTransformStatus InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile,
    const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags) override;
  virtual plTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
