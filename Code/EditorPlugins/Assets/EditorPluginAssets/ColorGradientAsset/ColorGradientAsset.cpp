#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plColorControlPoint, 2, plRTTIDefaultAllocator<plColorControlPoint>)
{
  PL_BEGIN_PROPERTIES
  {
    //PL_MEMBER_PROPERTY("Position", m_fPositionX),
    PL_MEMBER_PROPERTY("Tick", m_iTick),
    PL_MEMBER_PROPERTY("Red", m_Red)->AddAttributes(new plDefaultValueAttribute(255)),
    PL_MEMBER_PROPERTY("Green", m_Green)->AddAttributes(new plDefaultValueAttribute(255)),
    PL_MEMBER_PROPERTY("Blue", m_Blue)->AddAttributes(new plDefaultValueAttribute(255)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAlphaControlPoint, 2, plRTTIDefaultAllocator<plAlphaControlPoint>)
{
  PL_BEGIN_PROPERTIES
  {
    //PL_MEMBER_PROPERTY("Position", m_fPositionX),
    PL_MEMBER_PROPERTY("Tick", m_iTick),
    PL_MEMBER_PROPERTY("Alpha", m_Alpha)->AddAttributes(new plDefaultValueAttribute(255)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plIntensityControlPoint, 2, plRTTIDefaultAllocator<plIntensityControlPoint>)
{
  PL_BEGIN_PROPERTIES
  {
    //PL_MEMBER_PROPERTY("Position", m_fPositionX),
    PL_MEMBER_PROPERTY("Tick", m_iTick),
    PL_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;


PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plColorGradientAssetData, 2, plRTTIDefaultAllocator<plColorGradientAssetData>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("ColorCPs", m_ColorCPs),
    PL_ARRAY_MEMBER_PROPERTY("AlphaCPs", m_AlphaCPs),
    PL_ARRAY_MEMBER_PROPERTY("IntensityCPs", m_IntensityCPs),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plColorGradientAssetDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plColorControlPoint::SetTickFromTime(plTime time, plInt64 iFps)
{
  const plUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick = (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void plAlphaControlPoint::SetTickFromTime(plTime time, plInt64 iFps)
{
  const plUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick = (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void plIntensityControlPoint::SetTickFromTime(plTime time, plInt64 iFps)
{
  const plUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick = (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

plColorGradientAssetDocument::plColorGradientAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plColorGradientAssetData>(sDocumentPath, plAssetDocEngineConnection::None)
{
}

void plColorGradientAssetDocument::WriteResource(plStreamWriter& inout_stream) const
{
  const plColorGradientAssetData* pProp = GetProperties();

  plColorGradientResourceDescriptor desc;
  pProp->FillGradientData(desc.m_Gradient);
  desc.m_Gradient.SortControlPoints();

  desc.Save(inout_stream);
}

plInt64 plColorGradientAssetData::TickFromTime(plTime time)
{
  /// \todo Make this a property ?
  const plUInt32 uiFramesPerSecond = 60;
  const plUInt32 uiTicksPerStep = 4800 / uiFramesPerSecond;
  return (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void plColorGradientAssetData::FillGradientData(plColorGradient& out_result) const
{
  for (const auto& cp : m_ColorCPs)
  {
    out_result.AddColorControlPoint(cp.GetTickAsTime().GetSeconds(), plColorGammaUB(cp.m_Red, cp.m_Green, cp.m_Blue));
  }

  for (const auto& cp : m_AlphaCPs)
  {
    out_result.AddAlphaControlPoint(cp.GetTickAsTime().GetSeconds(), cp.m_Alpha);
  }

  for (const auto& cp : m_IntensityCPs)
  {
    out_result.AddIntensityControlPoint(cp.GetTickAsTime().GetSeconds(), cp.m_fIntensity);
  }
}

plColor plColorGradientAssetData::Evaluate(plInt64 iTick) const
{
  plColorGradient temp;
  {
    const plColorControlPoint* llhs = nullptr;
    const plColorControlPoint* lhs = nullptr;
    const plColorControlPoint* rhs = nullptr;
    const plColorControlPoint* rrhs = nullptr;
    FindNearestControlPoints(m_ColorCPs.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

    if (llhs)
      temp.AddColorControlPoint(llhs->GetTickAsTime().GetSeconds(), plColorGammaUB(llhs->m_Red, llhs->m_Green, llhs->m_Blue));
    if (lhs)
      temp.AddColorControlPoint(lhs->GetTickAsTime().GetSeconds(), plColorGammaUB(lhs->m_Red, lhs->m_Green, lhs->m_Blue));
    if (rhs)
      temp.AddColorControlPoint(rhs->GetTickAsTime().GetSeconds(), plColorGammaUB(rhs->m_Red, rhs->m_Green, rhs->m_Blue));
    if (rrhs)
      temp.AddColorControlPoint(rrhs->GetTickAsTime().GetSeconds(), plColorGammaUB(rrhs->m_Red, rrhs->m_Green, rrhs->m_Blue));
  }
  {
    const plAlphaControlPoint* llhs = nullptr;
    const plAlphaControlPoint* lhs = nullptr;
    const plAlphaControlPoint* rhs = nullptr;
    const plAlphaControlPoint* rrhs = nullptr;
    FindNearestControlPoints(m_AlphaCPs.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

    if (llhs)
      temp.AddAlphaControlPoint(llhs->GetTickAsTime().GetSeconds(), llhs->m_Alpha);
    if (lhs)
      temp.AddAlphaControlPoint(lhs->GetTickAsTime().GetSeconds(), lhs->m_Alpha);
    if (rhs)
      temp.AddAlphaControlPoint(rhs->GetTickAsTime().GetSeconds(), rhs->m_Alpha);
    if (rrhs)
      temp.AddAlphaControlPoint(rrhs->GetTickAsTime().GetSeconds(), rrhs->m_Alpha);
  }
  {
    const plIntensityControlPoint* llhs = nullptr;
    const plIntensityControlPoint* lhs = nullptr;
    const plIntensityControlPoint* rhs = nullptr;
    const plIntensityControlPoint* rrhs = nullptr;
    FindNearestControlPoints(m_IntensityCPs.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

    if (llhs)
      temp.AddIntensityControlPoint(llhs->GetTickAsTime().GetSeconds(), llhs->m_fIntensity);
    if (lhs)
      temp.AddIntensityControlPoint(lhs->GetTickAsTime().GetSeconds(), lhs->m_fIntensity);
    if (rhs)
      temp.AddIntensityControlPoint(rhs->GetTickAsTime().GetSeconds(), rhs->m_fIntensity);
    if (rrhs)
      temp.AddIntensityControlPoint(rrhs->GetTickAsTime().GetSeconds(), rrhs->m_fIntensity);
  }
  plColor color;
  //#TODO: This is rather slow as we eval lots of points but only need one
  temp.SortControlPoints();
  temp.Evaluate(iTick / 4800.0, color);
  return color;
}

plTransformStatus plColorGradientAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  WriteResource(stream);
  return plStatus(PL_SUCCESS);
}

plTransformStatus plColorGradientAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const plColorGradientAssetData* pProp = GetProperties();

  plImageHeader imgHeader;
  imgHeader.SetWidth(256);
  imgHeader.SetHeight(256);
  imgHeader.SetImageFormat(plImageFormat::R8G8B8A8_UNORM);
  plImage img;
  img.ResetAndAlloc(imgHeader);

  plColorGradient gradient;
  pProp->FillGradientData(gradient);
  gradient.SortControlPoints();

  double fMin, fMax;
  gradient.GetExtents(fMin, fMax);
  const double range = fMax - fMin;
  const double div = 1.0 / (img.GetWidth() - 1);
  const double factor = range * div;

  for (plUInt32 x = 0; x < img.GetWidth(); ++x)
  {
    const double pos = fMin + x * factor;

    plColorGammaUB color;
    gradient.EvaluateColor(pos, color);

    plUInt8 alpha;
    gradient.EvaluateAlpha(pos, alpha);
    const plColorLinearUB alphaColor = plColorLinearUB(alpha, alpha, alpha, 255);

    const float fAlphaFactor = plMath::ColorByteToFloat(alpha);
    plColor colorWithAlpha = color;
    colorWithAlpha.r *= fAlphaFactor;
    colorWithAlpha.g *= fAlphaFactor;
    colorWithAlpha.b *= fAlphaFactor;

    const plColorGammaUB colWithAlpha = colorWithAlpha;

    for (plUInt32 y = 0; y < img.GetHeight() / 4; ++y)
    {
      plColorGammaUB* pixel = img.GetPixelPointer<plColorGammaUB>(0, 0, 0, x, y);
      *pixel = alphaColor;
    }

    for (plUInt32 y = img.GetHeight() / 4; y < img.GetHeight() / 2; ++y)
    {
      plColorGammaUB* pixel = img.GetPixelPointer<plColorGammaUB>(0, 0, 0, x, y);
      *pixel = colWithAlpha;
    }

    for (plUInt32 y = img.GetHeight() / 2; y < img.GetHeight(); ++y)
    {
      plColorGammaUB* pixel = img.GetPixelPointer<plColorGammaUB>(0, 0, 0, x, y);
      *pixel = color;
    }
  }

  return SaveThumbnail(img, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plColorGradientAssetDataPatch_1_2 : public plGraphPatch
{
public:
  plColorGradientAssetDataPatch_1_2()
    : plGraphPatch("plColorGradientAssetData", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Color CPs", "ColorCPs");
    pNode->RenameProperty("Alpha CPs", "AlphaCPs");
    pNode->RenameProperty("Intensity CPs", "IntensityCPs");
  }
};

plColorGradientAssetDataPatch_1_2 g_plColorGradientAssetDataPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class plColorControlPoint_1_2 : public plGraphPatch
{
public:
  plColorControlPoint_1_2()
    : plGraphPatch("plColorControlPoint", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (plInt64)plMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

plColorControlPoint_1_2 g_plColorControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class plAlphaControlPoint_1_2 : public plGraphPatch
{
public:
  plAlphaControlPoint_1_2()
    : plGraphPatch("plAlphaControlPoint", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (plInt64)plMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

plAlphaControlPoint_1_2 g_plAlphaControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class plIntensityControlPoint_1_2 : public plGraphPatch
{
public:
  plIntensityControlPoint_1_2()
    : plGraphPatch("plIntensityControlPoint", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", (plInt64)plMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

plIntensityControlPoint_1_2 g_plIntensityControlPoint_1_2;
