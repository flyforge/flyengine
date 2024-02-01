#pragma once

#include <Foundation/Math/Math.h>

namespace plMath
{
  PL_ALWAYS_INLINE double GetCurveValue_Linear(double t)
  {
    return t;
  }

  PL_ALWAYS_INLINE double GetCurveValue_ConstantZero(double t)
  {
    return 0.0;
  }

  PL_ALWAYS_INLINE double GetCurveValue_ConstantOne(double t)
  {
    return 1.0;
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInSine(double t)
  {
    return 1.0 - cos((t * plMath::Pi<double>()) / 2.0);
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseOutSine(double t)
  {
    return sin((t * plMath::Pi<double>()) / 2.0);
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutSine(double t)
  {
    return -(cos(plMath::Pi<double>() * t) - 1.0) / 2.0;
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInQuad(double t)
  {
    return t * t;
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseOutQuad(double t)
  {
    return 1.0 - (1.0 - t) * (1.0 - t);
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutQuad(double t)
  {
    return t < 0.5 ? 2.0 * t * t : 1.0 - plMath::Pow(-2.0 * t + 2, 2.0) / 2;
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInCubic(double t)
  {
    return t * t * t;
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseOutCubic(double t)
  {
    return 1.0 - pow(1 - t, 3.0);
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutCubic(double t)
  {
    return t < 0.5 ? 4.0 * t * t * t : 1.0 - plMath::Pow(-2.0 * t + 2.0, 3.0) / 2.0;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInQuartic(double t)
  {
    return t * t * t * t;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseOutQuartic(double t)
  {
    return 1.0 - plMath::Pow(1.0 - t, 4.0);
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutQuartic(double t)
  {
    return t < 0.5 ? 8.0 * t * t * t * t : 1.0 - plMath::Pow(-2.0 * t + 2.0, 4.0) / 2.0;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInQuintic(double t)
  {
    return t * t * t * t * t;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseOutQuintic(double t)
  {
    return 1.0 - plMath::Pow(1.0 - t, 5.0);
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutQuintic(double t)
  {
    return t < 0.5 ? 16.0 * t * t * t * t * t : 1.0 - plMath::Pow(-2.0 * t + 2.0, 5.0) / 2.0;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInExpo(double t)
  {
    return t == 0 ? 0 : plMath::Pow(2.0, 10.0 * t - 10.0);
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseOutExpo(double t)
  {
    return t == 1.0 ? 1.0 : 1.0 - plMath::Pow(2.0, -10.0 * t);
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutExpo(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return t < 0.5 ? plMath::Pow(2.0, 20.0 * t - 10.0) / 2.0
                     : (2.0 - plMath::Pow(2.0, -20.0 * t + 10.0)) / 2.0;
    }
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInCirc(double t)
  {
    return 1.0 - plMath::Sqrt(1.0 - plMath::Pow(t, 2.0));
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseOutCirc(double t)
  {
    return plMath::Sqrt(1.0 - plMath::Pow(t - 1.0, 2.0));
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutCirc(double t)
  {
    return t < 0.5
             ? (1.0 - plMath::Sqrt(1.0 - plMath::Pow(2.0 * t, 2.0))) / 2.0
             : (plMath::Sqrt(1.0 - plMath::Pow(-2.0 * t + 2.0, 2.0)) + 1.0) / 2.0;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInBack(double t)
  {
    return 2.70158 * t * t * t - 1.70158 * t * t;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseOutBack(double t)
  {
    return 1 + 2.70158 * plMath::Pow(t - 1.0, 3.0) + 1.70158 * plMath::Pow(t - 1.0, 2.0);
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutBack(double t)
  {
    return t < 0.5
             ? (plMath::Pow(2.0 * t, 2.0) * (((1.70158 * 1.525) + 1.0) * 2 * t - (1.70158 * 1.525))) / 2.0
             : (plMath::Pow(2.0 * t - 2.0, 2.0) * (((1.70158 * 1.525) + 1.0) * (t * 2.0 - 2.0) + (1.70158 * 1.525)) + 2.0) / 2.0;
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseInElastic(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return -plMath::Pow(2.0, 10.0 * t - 10.0) * sin((t * 10.0 - 10.75) * ((2.0 * plMath::Pi<double>()) / 3.0));
    }
  }


  PL_ALWAYS_INLINE double GetCurveValue_EaseOutElastic(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return plMath::Pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * ((2.0 * plMath::Pi<double>()) / 3.0)) + 1.0;
    }
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutElastic(double t)
  {
    if (t == 0.0)
    {
      return 0.0;
    }
    else if (t == 1.0)
    {
      return 1.0;
    }
    else
    {
      return t < 0.5
               ? -(plMath::Pow(2.0, 20.0 * t - 10.0) * sin((20.0 * t - 11.125) * ((2 * plMath::Pi<double>()) / 4.5))) / 2.0
               : (plMath::Pow(2.0, -20.0 * t + 10.0) * sin((20.0 * t - 11.125) * ((2 * plMath::Pi<double>()) / 4.5))) / 2.0 + 1.0;
    }
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInBounce(double t)
  {
    return 1.0 - GetCurveValue_EaseOutBounce(1.0 - t);
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseOutBounce(double t)
  {
    if (t < 1.0 / 2.75)
    {
      return 7.5625 * t * t;
    }
    else if (t < 2.0 / 2.75)
    {
      t -= 1.5 / 2.75;
      return 7.5625 * t * t + 0.75;
    }
    else if (t < 2.5 / 2.75)
    {
      t -= 2.25 / 2.75;
      return 7.5625 * t * t + 0.9375;
    }
    else
    {
      t -= 2.625 / 2.75;
      return 7.5625 * t * t + 0.984375;
    }
  }

  PL_ALWAYS_INLINE double GetCurveValue_EaseInOutBounce(double t)
  {
    return t < 0.5
             ? (1.0 - GetCurveValue_EaseOutBounce(1.0 - 2.0 * t)) / 2.0
             : (1.0 + GetCurveValue_EaseOutBounce(2.0 * t - 1.0)) / 2.0;
  }

  PL_ALWAYS_INLINE double GetCurveValue_Conical(double t)
  {
    if (t < 0.2)
    {
      return 1.0f - plMath::Pow(1.0 - (t * 5.0), 4.0);
    }
    else
    {
      t = (t - 0.2) / 0.8; // normalize to 0-1 range

      return 1.0 - plMath::Pow(t, 2.0);
    }
  }

  PL_ALWAYS_INLINE double GetCurveValue_FadeInHoldFadeOut(double t)
  {
    if (t < 0.2)
    {
      return 1.0f - plMath::Pow(1.0 - (t * 5.0), 3.0);
    }
    else if (t > 0.8)
    {
      return 1.0 - plMath::Pow((t - 0.8) * 5.0, 3.0);
    }
    else
    {
      return 1.0;
    }
  }

  PL_ALWAYS_INLINE double GetCurveValue_FadeInFadeOut(double t)
  {
    if (t < 0.5)
    {
      return 1.0f - plMath::Pow(1.0 - (t * 2.0), 3.0);
    }
    else
    {
      return 1.0 - plMath::Pow((t - 0.5) * 2.0, 3.0);
    }
  }

  PL_ALWAYS_INLINE double GetCurveValue_Bell(double t)
  {
    if (t < 0.25)
    {
      return (plMath::Pow((t * 4.0), 3.0)) * 0.5;
    }
    else if (t < 0.5)
    {
      return (1.0f - plMath::Pow(1.0 - ((t - 0.25) * 4.0), 3.0)) * 0.5 + 0.5;
    }
    else if (t < 0.75)
    {
      return (1.0f - plMath::Pow(((t - 0.5) * 4.0), 3.0)) * 0.5 + 0.5;
    }
    else
    {
      return (plMath::Pow(1.0 - ((t - 0.75) * 4.0), 3.0)) * 0.5;
    }
  }
} // namespace plMath

// static
inline double plCurveFunction::GetValue(Enum function, double x)
{
  switch (function)
  {
    case Linear:
      return plMath::GetCurveValue_Linear(x);
    case ConstantZero:
      return plMath::GetCurveValue_ConstantZero(x);
    case ConstantOne:
      return plMath::GetCurveValue_ConstantOne(x);
    case EaseInSine:
      return plMath::GetCurveValue_EaseInSine(x);
    case EaseOutSine:
      return plMath::GetCurveValue_EaseOutSine(x);
    case EaseInOutSine:
      return plMath::GetCurveValue_EaseInOutSine(x);
    case EaseInQuad:
      return plMath::GetCurveValue_EaseInQuad(x);
    case EaseOutQuad:
      return plMath::GetCurveValue_EaseOutQuad(x);
    case EaseInOutQuad:
      return plMath::GetCurveValue_EaseInOutQuad(x);
    case EaseInCubic:
      return plMath::GetCurveValue_EaseInCubic(x);
    case EaseOutCubic:
      return plMath::GetCurveValue_EaseOutCubic(x);
    case EaseInOutCubic:
      return plMath::GetCurveValue_EaseInOutCubic(x);
    case EaseInQuartic:
      return plMath::GetCurveValue_EaseInQuartic(x);
    case EaseOutQuartic:
      return plMath::GetCurveValue_EaseOutQuartic(x);
    case EaseInOutQuartic:
      return plMath::GetCurveValue_EaseInOutQuartic(x);
    case EaseInQuintic:
      return plMath::GetCurveValue_EaseInQuintic(x);
    case EaseOutQuintic:
      return plMath::GetCurveValue_EaseOutQuintic(x);
    case EaseInOutQuintic:
      return plMath::GetCurveValue_EaseInOutQuintic(x);
    case EaseInExpo:
      return plMath::GetCurveValue_EaseInExpo(x);
    case EaseOutExpo:
      return plMath::GetCurveValue_EaseOutExpo(x);
    case EaseInOutExpo:
      return plMath::GetCurveValue_EaseInOutExpo(x);
    case EaseInCirc:
      return plMath::GetCurveValue_EaseInCirc(x);
    case EaseOutCirc:
      return plMath::GetCurveValue_EaseOutCirc(x);
    case EaseInOutCirc:
      return plMath::GetCurveValue_EaseInOutCirc(x);
    case EaseInBack:
      return plMath::GetCurveValue_EaseInBack(x);
    case EaseOutBack:
      return plMath::GetCurveValue_EaseOutBack(x);
    case EaseInOutBack:
      return plMath::GetCurveValue_EaseInOutBack(x);
    case EaseInElastic:
      return plMath::GetCurveValue_EaseInElastic(x);
    case EaseOutElastic:
      return plMath::GetCurveValue_EaseOutElastic(x);
    case EaseInOutElastic:
      return plMath::GetCurveValue_EaseInOutElastic(x);
    case EaseInBounce:
      return plMath::GetCurveValue_EaseInBounce(x);
    case EaseOutBounce:
      return plMath::GetCurveValue_EaseOutBounce(x);
    case EaseInOutBounce:
      return plMath::GetCurveValue_EaseInOutBounce(x);
    case Conical:
      return plMath::GetCurveValue_Conical(x);
    case FadeInHoldFadeOut:
      return plMath::GetCurveValue_FadeInHoldFadeOut(x);
    case FadeInFadeOut:
      return plMath::GetCurveValue_FadeInFadeOut(x);
    case Bell:
      return plMath::GetCurveValue_Bell(x);

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return 0.0;
}

// static
inline double plCurveFunction::GetValue(Enum function, double x, bool bInverse)
{
  double value = GetValue(function, x);

  return bInverse ? (1.0 - value) : value;
}
