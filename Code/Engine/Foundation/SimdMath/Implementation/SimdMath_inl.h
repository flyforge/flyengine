#pragma once

///\todo optimize these methods if needed

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Exp(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_exp_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Exp(f.x()), plMath::Exp(f.y()), plMath::Exp(f.z()), plMath::Exp(f.w()));
#endif
}

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Ln(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_log_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Ln(f.x()), plMath::Ln(f.y()), plMath::Ln(f.z()), plMath::Ln(f.w()));
#endif
}

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Log2(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_log2_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Log2(f.x()), plMath::Log2(f.y()), plMath::Log2(f.z()), plMath::Log2(f.w()));
#endif
}

// static
PLASMA_FORCE_INLINE plSimdVec4i plSimdMath::Log2i(const plSimdVec4i& i)
{
  return plSimdVec4i(plMath::Log2i(i.x()), plMath::Log2i(i.y()), plMath::Log2i(i.z()), plMath::Log2i(i.w()));
}

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Log10(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_log10_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Log10(f.x()), plMath::Log10(f.y()), plMath::Log10(f.z()), plMath::Log10(f.w()));
#endif
}

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Pow2(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_exp2_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Pow2(f.x()), plMath::Pow2(f.y()), plMath::Pow2(f.z()), plMath::Pow2(f.w()));
#endif
}

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Sin(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_sin_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Sin(plAngle::Radian(f.x())), plMath::Sin(plAngle::Radian(f.y())), plMath::Sin(plAngle::Radian(f.z())),
    plMath::Sin(plAngle::Radian(f.w())));
#endif
}

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Cos(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_cos_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Cos(plAngle::Radian(f.x())), plMath::Cos(plAngle::Radian(f.y())), plMath::Cos(plAngle::Radian(f.z())),
    plMath::Cos(plAngle::Radian(f.w())));
#endif
}

// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::Tan(const plSimdVec4f& f)
{
#if PLASMA_ENABLED(PLASMA_COMPILER_MSVC) && PLASMA_SIMD_IMPLEMENTATION == PLASMA_SIMD_IMPLEMENTATION_SSE
  return _mm_tan_ps(f.m_v);
#else
  return plSimdVec4f(plMath::Tan(plAngle::Radian(f.x())), plMath::Tan(plAngle::Radian(f.y())), plMath::Tan(plAngle::Radian(f.z())),
    plMath::Tan(plAngle::Radian(f.w())));
#endif
}

// static
PLASMA_ALWAYS_INLINE plSimdVec4f plSimdMath::ASin(const plSimdVec4f& f)
{
  return plSimdVec4f(plMath::Pi<float>() * 0.5f) - ACos(f);
}

// 4th order polynomial approximation
// 7 * 10^-5 radians precision
// Reference : Handbook of Mathematical Functions (chapter : Elementary Transcendental Functions), M. Abramowitz and I.A. Stegun, Ed.
// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::ACos(const plSimdVec4f& f)
{
  plSimdVec4f x1 = f.Abs();
  plSimdVec4f x2 = x1.CompMul(x1);
  plSimdVec4f x3 = x2.CompMul(x1);

  plSimdVec4f s = x1 * -0.2121144f + plSimdVec4f(1.5707288f);
  s += x2 * 0.0742610f;
  s += x3 * -0.0187293f;
  s = s.CompMul((plSimdVec4f(1.0f) - x1).GetSqrt());

  return plSimdVec4f::Select(f >= plSimdVec4f::ZeroVector(), s, plSimdVec4f(plMath::Pi<float>()) - s);
}

// Reference: https://seblagarde.wordpress.com/2014/12/01/inverse-trigonometric-functions-gpu-optimization-for-amd-gcn-architecture/
// static
PLASMA_FORCE_INLINE plSimdVec4f plSimdMath::ATan(const plSimdVec4f& f)
{
  plSimdVec4f x = f.Abs();
  plSimdVec4f t0 = plSimdVec4f::Select(x < plSimdVec4f(1.0f), x, x.GetReciprocal());
  plSimdVec4f t1 = t0.CompMul(t0);
  plSimdVec4f poly = plSimdVec4f(0.0872929f);
  poly = plSimdVec4f(-0.301895f) + poly.CompMul(t1);
  poly = plSimdVec4f(1.0f) + poly.CompMul(t1);
  poly = poly.CompMul(t0);
  t0 = plSimdVec4f::Select(x < plSimdVec4f(1.0f), poly, plSimdVec4f(plMath::Pi<float>() * 0.5f) - poly);

  return plSimdVec4f::Select(f < plSimdVec4f::ZeroVector(), -t0, t0);
}
