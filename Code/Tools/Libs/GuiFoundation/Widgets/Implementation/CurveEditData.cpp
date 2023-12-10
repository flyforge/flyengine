#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/Widgets/CurveEditData.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plCurveTangentMode, 1)
PLASMA_ENUM_CONSTANTS(plCurveTangentMode::Bezier, plCurveTangentMode::FixedLength, plCurveTangentMode::Linear, plCurveTangentMode::Auto)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCurveControlPointData, 5, plRTTIDefaultAllocator<plCurveControlPointData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Tick", m_iTick),
    PLASMA_MEMBER_PROPERTY("Value", m_fValue),
    PLASMA_MEMBER_PROPERTY("LeftTangent", m_LeftTangent)->AddAttributes(new plDefaultValueAttribute(plVec2(-0.1f, 0))),
    PLASMA_MEMBER_PROPERTY("RightTangent", m_RightTangent)->AddAttributes(new plDefaultValueAttribute(plVec2(+0.1f, 0))),
    PLASMA_MEMBER_PROPERTY("Linked", m_bTangentsLinked)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ENUM_MEMBER_PROPERTY("LeftTangentMode", plCurveTangentMode, m_LeftTangentMode),
    PLASMA_ENUM_MEMBER_PROPERTY("RightTangentMode", plCurveTangentMode, m_RightTangentMode),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSingleCurveData, 3, plRTTIDefaultAllocator<plSingleCurveData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Color", m_CurveColor)->AddAttributes(new plDefaultValueAttribute(plColorScheme::LightUI(plColorScheme::Lime))),
    PLASMA_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCurveGroupData, 2, plRTTIDefaultAllocator<plCurveGroupData>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new plDefaultValueAttribute(60)),
    PLASMA_ARRAY_MEMBER_PROPERTY("Curves", m_Curves)->AddFlags(plPropertyFlags::PointerOwner),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plCurveExtentsAttribute, 1, plRTTIDefaultAllocator<plCurveExtentsAttribute>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("LowerExtent", m_fLowerExtent),
    PLASMA_MEMBER_PROPERTY("UpperExtent", m_fUpperExtent),
    PLASMA_MEMBER_PROPERTY("LowerExtentFixed", m_bLowerExtentFixed),
    PLASMA_MEMBER_PROPERTY("UpperExtentFixed", m_bUpperExtentFixed),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_CONSTRUCTOR_PROPERTY(float, bool, float, bool),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plCurveExtentsAttribute::plCurveExtentsAttribute(double fLowerExtent, bool bLowerExtentFixed, double fUpperExtent, bool bUpperExtentFixed)
{
  m_fLowerExtent = fLowerExtent;
  m_fUpperExtent = fUpperExtent;
  m_bLowerExtentFixed = bLowerExtentFixed;
  m_bUpperExtentFixed = bUpperExtentFixed;
}

void plCurveControlPointData::SetTickFromTime(plTime time, plInt64 iFps)
{
  const plUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick = (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

plCurveGroupData::~plCurveGroupData()
{
  Clear();
}

void plCurveGroupData::CloneFrom(const plCurveGroupData& rhs)
{
  Clear();

  m_bOwnsData = true;
  m_uiFramesPerSecond = rhs.m_uiFramesPerSecond;
  m_Curves.SetCount(rhs.m_Curves.GetCount());

  for (plUInt32 i = 0; i < m_Curves.GetCount(); ++i)
  {
    m_Curves[i] = PLASMA_DEFAULT_NEW(plSingleCurveData);
    *m_Curves[i] = *(rhs.m_Curves[i]);
  }
}

void plCurveGroupData::Clear()
{
  m_uiFramesPerSecond = 60;

  if (m_bOwnsData)
  {
    m_bOwnsData = false;

    for (plUInt32 i = 0; i < m_Curves.GetCount(); ++i)
    {
      PLASMA_DEFAULT_DELETE(m_Curves[i]);
    }
  }

  m_Curves.Clear();
}

plInt64 plCurveGroupData::TickFromTime(plTime time) const
{
  const plUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

static void ConvertControlPoint(const plCurveControlPointData& cp, plCurve1D& out_result)
{
  auto& ccp = out_result.AddControlPoint(cp.GetTickAsTime().GetSeconds());
  ccp.m_Position.y = cp.m_fValue;
  ccp.m_LeftTangent = cp.m_LeftTangent;
  ccp.m_RightTangent = cp.m_RightTangent;
  ccp.m_TangentModeLeft = cp.m_LeftTangentMode;
  ccp.m_TangentModeRight = cp.m_RightTangentMode;
}

void plSingleCurveData::ConvertToRuntimeData(plCurve1D& out_result) const
{
  out_result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    ConvertControlPoint(cp, out_result);
  }
}

double plSingleCurveData::Evaluate(plInt64 iTick) const
{
  plCurve1D temp;
  const plCurveControlPointData* llhs = nullptr;
  const plCurveControlPointData* lhs = nullptr;
  const plCurveControlPointData* rhs = nullptr;
  const plCurveControlPointData* rrhs = nullptr;
  FindNearestControlPoints(m_ControlPoints.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

  if (llhs)
    ConvertControlPoint(*llhs, temp);
  if (lhs)
    ConvertControlPoint(*lhs, temp);
  if (rhs)
    ConvertControlPoint(*rhs, temp);
  if (rrhs)
    ConvertControlPoint(*rrhs, temp);

  //#TODO: This is rather slow as we eval lots of points but only need one
  temp.CreateLinearApproximation();
  return temp.Evaluate(iTick / 4800.0);
}

void plCurveGroupData::ConvertToRuntimeData(plUInt32 uiCurveIdx, plCurve1D& out_result) const
{
  m_Curves[uiCurveIdx]->ConvertToRuntimeData(out_result);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plCurve1DControlPoint_2_3 : public plGraphPatch
{
public:
  plCurve1DControlPoint_2_3()
    : plGraphPatch("plCurve1DControlPoint", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Point");
    if (pPoint && pPoint->m_Value.IsA<plVec2>())
    {
      plVec2 pt = pPoint->m_Value.Get<plVec2>();
      pNode->AddProperty("Time", (double)plMath::Max(0.0f, pt.x));
      pNode->AddProperty("Value", (double)pt.y);
      pNode->AddProperty("LeftTangentMode", (plUInt32)plCurveTangentMode::Bezier);
      pNode->AddProperty("RightTangentMode", (plUInt32)plCurveTangentMode::Bezier);
    }
  }
};

plCurve1DControlPoint_2_3 g_plCurve1DControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class plCurve1DControlPoint_3_4 : public plGraphPatch
{
public:
  plCurve1DControlPoint_3_4()
    : plGraphPatch("plCurve1DControlPoint", 4)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Time");
    if (pPoint && pPoint->m_Value.IsA<double>())
    {
      const double fTime = pPoint->m_Value.Get<double>();
      pNode->AddProperty("Tick", (plInt64)plMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

plCurve1DControlPoint_3_4 g_plCurve1DControlPoint_3_4;

//////////////////////////////////////////////////////////////////////////

class plCurve1DControlPoint_4_5 : public plGraphPatch
{
public:
  plCurve1DControlPoint_4_5()
    : plGraphPatch("plCurve1DControlPoint", 5)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("plCurveControlPointData");
  }
};

plCurve1DControlPoint_4_5 g_plCurve1DControlPoint_4_5;

//////////////////////////////////////////////////////////////////////////

class plCurve1DData_2_3 : public plGraphPatch
{
public:
  plCurve1DData_2_3()
    : plGraphPatch("plCurve1DData", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("plSingleCurveData");
  }
};

plCurve1DData_2_3 g_plCurve1DData_2_3;

//////////////////////////////////////////////////////////////////////////

class plCurve1DAssetData_1_2 : public plGraphPatch
{
public:
  plCurve1DAssetData_1_2()
    : plGraphPatch("plCurve1DAssetData", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("plCurveGroupData");
  }
};

plCurve1DAssetData_1_2 g_plCurve1DAssetData_1_2;
