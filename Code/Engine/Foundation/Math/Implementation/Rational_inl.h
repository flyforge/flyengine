
PLASMA_ALWAYS_INLINE plRational::plRational()

  = default;

PLASMA_ALWAYS_INLINE plRational::plRational(plUInt32 uiNumerator, plUInt32 uiDenominator)
  : m_uiNumerator(uiNumerator)
  , m_uiDenominator(uiDenominator)
{
}

PLASMA_ALWAYS_INLINE bool plRational::IsIntegral() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return true;

  return ((m_uiNumerator / m_uiDenominator) * m_uiDenominator) == m_uiNumerator;
}

PLASMA_ALWAYS_INLINE bool plRational::operator==(const plRational& other) const
{
  return m_uiNumerator == other.m_uiNumerator && m_uiDenominator == other.m_uiDenominator;
}

PLASMA_ALWAYS_INLINE bool plRational::operator!=(const plRational& other) const
{
  return m_uiNumerator != other.m_uiNumerator || m_uiDenominator != other.m_uiDenominator;
}

PLASMA_ALWAYS_INLINE plUInt32 plRational::GetNumerator() const
{
  return m_uiNumerator;
}

PLASMA_ALWAYS_INLINE plUInt32 plRational::GetDenominator() const
{
  return m_uiDenominator;
}

PLASMA_ALWAYS_INLINE plUInt32 plRational::GetIntegralResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0;

  return m_uiNumerator / m_uiDenominator;
}

PLASMA_ALWAYS_INLINE double plRational::GetFloatingPointResult() const
{
  if (m_uiNumerator == 0 && m_uiDenominator == 0)
    return 0.0;

  return static_cast<double>(m_uiNumerator) / static_cast<double>(m_uiDenominator);
}

PLASMA_ALWAYS_INLINE bool plRational::IsValid() const
{
  return m_uiDenominator != 0 || (m_uiNumerator == 0 && m_uiDenominator == 0);
}

PLASMA_ALWAYS_INLINE plRational plRational::ReduceIntegralFraction() const
{
  PLASMA_ASSERT_DEV(IsValid() && IsIntegral(), "ReduceIntegralFraction can only be called on valid, integral rational numbers");

  return plRational(m_uiNumerator / m_uiDenominator, 1);
}
