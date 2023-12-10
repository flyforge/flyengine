
plUuid::plUuid()
  : m_uiHigh(0)
  , m_uiLow(0)
{
}

bool plUuid::operator==(const plUuid& other) const
{
  return m_uiHigh == other.m_uiHigh && m_uiLow == other.m_uiLow;
}

bool plUuid::operator!=(const plUuid& other) const
{
  return m_uiHigh != other.m_uiHigh || m_uiLow != other.m_uiLow;
}

bool plUuid::operator<(const plUuid& other) const
{
  if (m_uiHigh < other.m_uiHigh)
    return true;
  if (m_uiHigh > other.m_uiHigh)
    return false;

  return m_uiLow < other.m_uiLow;
}

bool plUuid::IsValid() const
{
  return m_uiHigh != 0 || m_uiLow != 0;
}

void plUuid::CombineWithSeed(const plUuid& seed)
{
  m_uiHigh += seed.m_uiHigh;
  m_uiLow += seed.m_uiLow;
}

void plUuid::RevertCombinationWithSeed(const plUuid& seed)
{
  m_uiHigh -= seed.m_uiHigh;
  m_uiLow -= seed.m_uiLow;
}

void plUuid::HashCombine(const plUuid& guid)
{
  m_uiHigh = plHashingUtils::xxHash64(&guid.m_uiHigh, sizeof(plUInt64), m_uiHigh);
  m_uiLow = plHashingUtils::xxHash64(&guid.m_uiLow, sizeof(plUInt64), m_uiLow);
}

template <>
struct plHashHelper<plUuid>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const plUuid& value) { return plHashingUtils::xxHash32(&value, sizeof(plUuid)); }

  PLASMA_ALWAYS_INLINE static bool Equal(const plUuid& a, const plUuid& b) { return a == b; }
};
