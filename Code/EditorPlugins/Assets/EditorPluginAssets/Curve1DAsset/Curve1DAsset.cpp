#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCurve1DAssetDocument, 3, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plCurve1DAssetDocument::plCurve1DAssetDocument(const char* szDocumentPath)
  : plSimpleAssetDocument<plCurveGroupData>(szDocumentPath, plAssetDocEngineConnection::None)
{
}

plCurve1DAssetDocument::~plCurve1DAssetDocument() = default;

void plCurve1DAssetDocument::FillCurve(plUInt32 uiCurveIdx, plCurve1D& out_Result) const
{
  const plCurveGroupData* pProp = static_cast<const plCurveGroupData*>(GetProperties());
  pProp->ConvertToRuntimeData(uiCurveIdx, out_Result);
}

plUInt32 plCurve1DAssetDocument::GetCurveCount() const
{
  const plCurveGroupData* pProp = GetProperties();
  return pProp->m_Curves.GetCount();
}

void plCurve1DAssetDocument::WriteResource(plStreamWriter& stream) const
{
  const plCurveGroupData* pProp = GetProperties();

  plCurve1DResourceDescriptor desc;
  desc.m_Curves.SetCount(pProp->m_Curves.GetCount());

  for (plUInt32 i = 0; i < pProp->m_Curves.GetCount(); ++i)
  {
    FillCurve(i, desc.m_Curves[i]);
    desc.m_Curves[i].SortControlPoints();
  }

  desc.Save(stream);
}

plTransformStatus plCurve1DAssetDocument::InternalTransformAsset(plStreamWriter& stream, const char* szOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  WriteResource(stream);
  return plStatus(PLASMA_SUCCESS);
}

plTransformStatus plCurve1DAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const plCurveGroupData* pProp = GetProperties();

  QImage qimg(plThumbnailSize, plThumbnailSize, QImage::Format_RGBA8888);
  qimg.fill(QColor(50, 50, 50));

  QPainter p(&qimg);
  QPainter* painter = &p;
  painter->setBrush(Qt::NoBrush);
  painter->setRenderHint(QPainter::Antialiasing);

  if (!pProp->m_Curves.IsEmpty())
  {

    double fExtentsMin, fExtentsMax;
    double fExtremesMin, fExtremesMax;

    for (plUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      plCurve1D curve;
      FillCurve(curveIdx, curve);

      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      double fMin, fMax;
      curve.QueryExtents(fMin, fMax);

      double fMin2, fMax2;
      curve.QueryExtremeValues(fMin2, fMax2);

      if (curveIdx == 0)
      {
        fExtentsMin = fMin;
        fExtentsMax = fMax;
        fExtremesMin = fMin2;
        fExtremesMax = fMax2;
      }
      else
      {
        fExtentsMin = plMath::Min(fExtentsMin, fMin);
        fExtentsMax = plMath::Max(fExtentsMax, fMax);
        fExtremesMin = plMath::Min(fExtremesMin, fMin2);
        fExtremesMax = plMath::Max(fExtremesMax, fMax2);
      }
    }

    const float range = fExtentsMax - fExtentsMin;
    const float div = 1.0f / (qimg.width() - 1);
    const float factor = range * div;

    const float lowValue = (fExtremesMin > 0) ? 0.0f : fExtremesMin;
    const float highValue = (fExtremesMax < 0) ? 0.0f : fExtremesMax;

    const float range2 = highValue - lowValue;

    for (plUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      QPainterPath path;

      plCurve1D curve;
      FillCurve(curveIdx, curve);
      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      const QColor curColor = plToQtColor(pProp->m_Curves[curveIdx]->m_CurveColor);
      QPen pen(curColor, 8.0f);
      painter->setPen(pen);

      for (plUInt32 x = 0; x < (plUInt32)qimg.width(); ++x)
      {
        const float pos = fExtentsMin + x * factor;
        const float value = 1.0f - (curve.Evaluate(pos) - lowValue) / range2;

        const plUInt32 y = plMath::Clamp<plUInt32>(qimg.height() * value, 0, qimg.height() - 1);

        if (x == 0)
          path.moveTo(x, y);
        else
          path.lineTo(x, y);
      }

      painter->drawPath(path);
    }
  }

  return SaveThumbnail(qimg, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plCurve1DControlPointPatch_1_2 : public plGraphPatch
{
public:
  plCurve1DControlPointPatch_1_2()
    : plGraphPatch("plCurve1DControlPoint", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Left Tangent", "LeftTangent");
    pNode->RenameProperty("Right Tangent", "RightTangent");
  }
};

plCurve1DControlPointPatch_1_2 g_plCurve1DControlPointPatch_1_2;


class plCurve1DDataPatch_1_2 : public plGraphPatch
{
public:
  plCurve1DDataPatch_1_2()
    : plGraphPatch("plCurve1DData", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override { pNode->RenameProperty("Control Points", "ControlPoints"); }
};

plCurve1DDataPatch_1_2 g_plCurve1DDataPatch_1_2;
