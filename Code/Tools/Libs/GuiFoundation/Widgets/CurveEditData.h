#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Tracks/Curve1D.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plCurve1D;

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GUIFOUNDATION_DLL, plCurveTangentMode);

template <typename T>
void FindNearestControlPoints(plArrayPtr<T> cps, plInt64 iTick, T*& llhs, T*& lhs, T*& rhs, T*& rrhs)
{
  llhs = nullptr;
  lhs = nullptr;
  rhs = nullptr;
  rrhs = nullptr;
  plInt64 lhsTick = plMath::MinValue<plInt64>();
  plInt64 llhsTick = plMath::MinValue<plInt64>();
  plInt64 rhsTick = plMath::MaxValue<plInt64>();
  plInt64 rrhsTick = plMath::MaxValue<plInt64>();

  for (decltype(auto) cp : cps)
  {
    if (cp.m_iTick <= iTick)
    {
      if (cp.m_iTick > lhsTick)
      {
        llhs = lhs;
        llhsTick = lhsTick;

        lhs = &cp;
        lhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick > llhsTick)
      {
        llhs = &cp;
        llhsTick = cp.m_iTick;
      }
    }

    if (cp.m_iTick > iTick)
    {
      if (cp.m_iTick < rhsTick)
      {
        rrhs = rhs;
        rrhsTick = rhsTick;

        rhs = &cp;
        rhsTick = cp.m_iTick;
      }
      else if (cp.m_iTick < rrhsTick)
      {
        rrhs = &cp;
        rrhsTick = cp.m_iTick;
      }
    }
  }
}

class PLASMA_GUIFOUNDATION_DLL plCurveControlPointData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCurveControlPointData, plReflectedClass);

public:
  plTime GetTickAsTime() const { return plTime::Seconds(m_iTick / 4800.0); }
  void SetTickFromTime(plTime time, plInt64 fps);

  plInt64 m_iTick; // 4800 ticks per second
  double m_fValue;
  plVec2 m_LeftTangent = plVec2(-0.1f, 0.0f);
  plVec2 m_RightTangent = plVec2(+0.1f, 0.0f);
  bool m_bTangentsLinked = true;
  plEnum<plCurveTangentMode> m_LeftTangentMode;
  plEnum<plCurveTangentMode> m_RightTangentMode;
};

class PLASMA_GUIFOUNDATION_DLL plSingleCurveData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSingleCurveData, plReflectedClass);

public:
  plColorGammaUB m_CurveColor;
  plDynamicArray<plCurveControlPointData> m_ControlPoints;

  void ConvertToRuntimeData(plCurve1D& out_Result) const;
  double Evaluate(plInt64 uiTick) const;
};

class PLASMA_GUIFOUNDATION_DLL plCurveExtentsAttribute : public plPropertyAttribute
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCurveExtentsAttribute, plPropertyAttribute);

public:
  plCurveExtentsAttribute() = default;
  plCurveExtentsAttribute(double fLowerExtent, bool bLowerExtentFixed, double fUpperExtent, bool bUpperExtentFixed);

  double m_fLowerExtent = 0.0;
  double m_fUpperExtent = 1.0;
  bool m_bLowerExtentFixed = false;
  bool m_bUpperExtentFixed = false;
};


class PLASMA_GUIFOUNDATION_DLL plCurveGroupData : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCurveGroupData, plReflectedClass);

public:
  plCurveGroupData() = default;
  plCurveGroupData(const plCurveGroupData& rhs) = delete;
  ~plCurveGroupData();
  plCurveGroupData& operator=(const plCurveGroupData& rhs) = delete;

  /// \brief Makes a deep copy of rhs.
  void CloneFrom(const plCurveGroupData& rhs);

  /// \brief Clears the curve and deallocates the curve data, if it is owned (e.g. if it was created through CloneFrom())
  void Clear();

  /// Can be set to false for cases where the instance is only supposed to act like a container for passing curve pointers around
  bool m_bOwnsData = true;
  plDynamicArray<plSingleCurveData*> m_Curves;
  plUInt16 m_uiFramesPerSecond = 60;

  plInt64 TickFromTime(plTime time) const;

  void ConvertToRuntimeData(plUInt32 uiCurveIdx, plCurve1D& out_Result) const;
};

struct PLASMA_GUIFOUNDATION_DLL plSelectedCurveCP
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt16 m_uiCurve;
  plUInt16 m_uiPoint;
};
