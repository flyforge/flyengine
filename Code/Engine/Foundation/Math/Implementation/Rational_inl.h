
PL_ALWAYS_INLINE plRational::plRational()

  = default;

PL_ALWAYS_INLINE plRational::plRational(plUInt32 uiNumerator, plUInt32 uiDenominator)
  : m_uiNumerator(uiNumerator)
  , m_uiDenominator(uiDenominator)
{
}

PL_ALWAYS_INLINE bool plRational::IsIntegral() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return true;

  return ((m_uiNumerator / m_uiDenominator) * m_uiDenominator) == m_uiNumerator;
}

PL_ALWAYS_INLINE bool plRational::operator==(const plRational& other) const
{
  return m_uiNumerator == other.m_uiNumerator && m_uiDenominator == other.m_uiDenominator;
}

PL_ALWAYS_INLINE bool plRational::operator!=(const plRational& other) const
{
  return m_uiNumerator != other.m_uiNumerator || m_uiDenominator != other.m_uiDenominator;
}

PL_ALWAYS_INLINE plUInt32 plRational::GetNumerator() const
{
  return m_uiNumerator;
}

PL_ALWAYS_INLINE plUInt32 plRational::GetDenominator() const
{
  return m_uiDenominator;
}

PL_ALWAYS_INLINE plUInt32 plRational::GetIntegralResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0;

  return m_uiNumerator / m_uiDenominator;
}

PL_ALWAYS_INLINE double plRational::GetFloatingPointResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0.0;

  return static_cast<double>(m_uiNumerator) / static_cast<double>(m_uiDenominator);
}

PL_ALWAYS_INLINE bool plRational::IsValid() const
{
  return m_uiDenominator != 0 || (m_uiNumerator == 0 && m_uiDenominator == 0);
}

PL_ALWAYS_INLINE plRational plRational::ReduceIntegralFraction() const
{
  PL_ASSERT_DEV(IsValid() && IsIntegral(), "ReduceIntegralFraction can only be called on valid, integral rational numbers");

  return plRational(m_uiNumerator / m_uiDenominator, 1);
}
