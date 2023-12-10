#pragma once

#include <Foundation/SimdMath/SimdVec4i.h>

struct plSimdMath
{
  static plSimdVec4f Exp(const plSimdVec4f& f);
  static plSimdVec4f Ln(const plSimdVec4f& f);
  static plSimdVec4f Log2(const plSimdVec4f& f);
  static plSimdVec4i Log2i(const plSimdVec4i& i);
  static plSimdVec4f Log10(const plSimdVec4f& f);
  static plSimdVec4f Pow2(const plSimdVec4f& f);

  static plSimdVec4f Sin(const plSimdVec4f& f);
  static plSimdVec4f Cos(const plSimdVec4f& f);
  static plSimdVec4f Tan(const plSimdVec4f& f);

  static plSimdVec4f ASin(const plSimdVec4f& f);
  static plSimdVec4f ACos(const plSimdVec4f& f);
  static plSimdVec4f ATan(const plSimdVec4f& f);
};

#include <Foundation/SimdMath/Implementation/SimdMath_inl.h>
