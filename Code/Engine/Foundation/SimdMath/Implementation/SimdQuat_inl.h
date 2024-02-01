#pragma once

PL_ALWAYS_INLINE plSimdQuat::plSimdQuat() = default;

PL_ALWAYS_INLINE plSimdQuat::plSimdQuat(const plSimdVec4f& v)
  : m_v(v)
{
}

PL_ALWAYS_INLINE const plSimdQuat plSimdQuat::MakeIdentity()
{
  return plSimdQuat(plSimdVec4f(0.0f, 0.0f, 0.0f, 1.0f));
}

PL_ALWAYS_INLINE plSimdQuat plSimdQuat::MakeFromElements(plSimdFloat x, plSimdFloat y, plSimdFloat z, plSimdFloat w)
{
  return plSimdQuat(plSimdVec4f(x, y, z, w));
}

inline plSimdQuat plSimdQuat::MakeFromAxisAndAngle(const plSimdVec4f& vRotationAxis, const plSimdFloat& fAngle)
{
  ///\todo optimize
  const plAngle halfAngle = plAngle::MakeFromRadian(fAngle) * 0.5f;
  float s = plMath::Sin(halfAngle);
  float c = plMath::Cos(halfAngle);

  plSimdQuat res;
  res.m_v = vRotationAxis * s;
  res.m_v.SetW(c);
  return res;
}

PL_ALWAYS_INLINE void plSimdQuat::Normalize()
{
  m_v.Normalize<4>();
}

inline plResult plSimdQuat::GetRotationAxisAndAngle(plSimdVec4f& ref_vAxis, plSimdFloat& ref_fAngle, const plSimdFloat& fEpsilon) const
{
  ///\todo optimize
  const plAngle acos = plMath::ACos(m_v.w());
  const float d = plMath::Sin(acos);

  if (d < fEpsilon)
  {
    ref_vAxis.Set(1.0f, 0.0f, 0.0f, 0.0f);
  }
  else
  {
    ref_vAxis = m_v / d;
  }

  ref_fAngle = acos * 2;

  return PL_SUCCESS;
}

PL_ALWAYS_INLINE plSimdMat4f plSimdQuat::GetAsMat4() const
{
  const plSimdVec4f xyz = m_v;
  const plSimdVec4f x2y2z2 = xyz + xyz;
  const plSimdVec4f xx2yy2zz2 = x2y2z2.CompMul(xyz);

  // diagonal terms
  // 1 - (yy2 + zz2)
  // 1 - (xx2 + zz2)
  // 1 - (xx2 + yy2)
  const plSimdVec4f yy2_xx2_xx2 = xx2yy2zz2.Get<plSwizzle::YXXX>();
  const plSimdVec4f zz2_zz2_yy2 = xx2yy2zz2.Get<plSwizzle::ZZYX>();
  plSimdVec4f diagonal = plSimdVec4f(1.0f) - (yy2_xx2_xx2 + zz2_zz2_yy2);
  diagonal.SetW(plSimdFloat::MakeZero());

  // non diagonal terms
  // xy2 +- wz2
  // yz2 +- wx2
  // xz2 +- wy2
  const plSimdVec4f x_y_x = xyz.Get<plSwizzle::XYXX>();
  const plSimdVec4f y2_z2_z2 = x2y2z2.Get<plSwizzle::YZZX>();
  const plSimdVec4f base = x_y_x.CompMul(y2_z2_z2);

  const plSimdVec4f z2_x2_y2 = x2y2z2.Get<plSwizzle::ZXYX>();
  const plSimdVec4f offset = z2_x2_y2 * m_v.w();

  const plSimdVec4f adds = base + offset;
  const plSimdVec4f subs = base - offset;

  // final matrix layout
  // col0 = (diaX, addX, subZ, diaW)
  const plSimdVec4f addX_u_diaX_u = adds.GetCombined<plSwizzle::XXXX>(diagonal);
  const plSimdVec4f subZ_u_diaW_u = subs.GetCombined<plSwizzle::ZXWX>(diagonal);
  const plSimdVec4f col0 = addX_u_diaX_u.GetCombined<plSwizzle::ZXXZ>(subZ_u_diaW_u);

  // col1 = (subX, diaY, addY, diaW)
  const plSimdVec4f subX_u_diaY_u = subs.GetCombined<plSwizzle::XXYX>(diagonal);
  const plSimdVec4f addY_u_diaW_u = adds.GetCombined<plSwizzle::YXWX>(diagonal);
  const plSimdVec4f col1 = subX_u_diaY_u.GetCombined<plSwizzle::XZXZ>(addY_u_diaW_u);

  // col2 = (addZ, subY, diaZ, diaW)
  const plSimdVec4f addZ_u_subY_u = adds.GetCombined<plSwizzle::ZXYX>(subs);
  const plSimdVec4f col2 = addZ_u_subY_u.GetCombined<plSwizzle::XZZW>(diagonal);

  return plSimdMat4f::MakeFromColumns(col0, col1, col2, plSimdVec4f(0, 0, 0, 1));
}

PL_ALWAYS_INLINE bool plSimdQuat::IsValid(const plSimdFloat& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

PL_ALWAYS_INLINE bool plSimdQuat::IsNaN() const
{
  return m_v.IsNaN<4>();
}

PL_ALWAYS_INLINE plSimdQuat plSimdQuat::operator-() const
{
  return plSimdQuat(m_v.FlipSign(plSimdVec4b(true, true, true, false)));
}

PL_ALWAYS_INLINE plSimdVec4f plSimdQuat::operator*(const plSimdVec4f& v) const
{
  plSimdVec4f t = m_v.CrossRH(v);
  t += t;
  return v + t * m_v.w() + m_v.CrossRH(t);
}

PL_ALWAYS_INLINE plSimdQuat plSimdQuat::operator*(const plSimdQuat& q2) const
{
  plSimdQuat q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.CrossRH(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

PL_ALWAYS_INLINE bool plSimdQuat::operator==(const plSimdQuat& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

PL_ALWAYS_INLINE bool plSimdQuat::operator!=(const plSimdQuat& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}
