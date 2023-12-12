
template <typename T>
PLASMA_ALWAYS_INLINE plAtomicInteger<T>::plAtomicInteger()
  : m_value(0)
{
}

template <typename T>
PLASMA_ALWAYS_INLINE plAtomicInteger<T>::plAtomicInteger(T value)
  : m_value(value)
{
}

template <typename T>
PLASMA_ALWAYS_INLINE plAtomicInteger<T>::plAtomicInteger(const plAtomicInteger<T>& value)
  : m_value(value.m_value)
{
}

template <typename T>
PLASMA_ALWAYS_INLINE plAtomicInteger<T>& plAtomicInteger<T>::operator=(const T value)
{
  m_value = value;
  return *this;
}

template <typename T>
PLASMA_ALWAYS_INLINE plAtomicInteger<T>& plAtomicInteger<T>::operator=(const plAtomicInteger<T>& value)
{
  m_value = value.m_value;
  return *this;
}

template <typename T>
PLASMA_ALWAYS_INLINE T plAtomicInteger<T>::Increment()
{
  return plAtomicUtils::Increment(m_value);
}

template <typename T>
PLASMA_ALWAYS_INLINE T plAtomicInteger<T>::Decrement()
{
  return plAtomicUtils::Decrement(m_value);
}

template <typename T>
PLASMA_ALWAYS_INLINE T plAtomicInteger<T>::PostIncrement()
{
  return plAtomicUtils::PostIncrement(m_value);
}

template <typename T>
PLASMA_ALWAYS_INLINE T plAtomicInteger<T>::PostDecrement()
{
  return plAtomicUtils::PostDecrement(m_value);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plAtomicInteger<T>::Add(T x)
{
  plAtomicUtils::Add(m_value, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plAtomicInteger<T>::Subtract(T x)
{
  plAtomicUtils::Add(m_value, -x);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plAtomicInteger<T>::And(T x)
{
  plAtomicUtils::And(m_value, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plAtomicInteger<T>::Or(T x)
{
  plAtomicUtils::Or(m_value, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plAtomicInteger<T>::Xor(T x)
{
  plAtomicUtils::Xor(m_value, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plAtomicInteger<T>::Min(T x)
{
  plAtomicUtils::Min(m_value, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE void plAtomicInteger<T>::Max(T x)
{
  plAtomicUtils::Max(m_value, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE T plAtomicInteger<T>::Set(T x)
{
  return plAtomicUtils::Set(m_value, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE bool plAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return plAtomicUtils::TestAndSet(m_value, expected, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE T plAtomicInteger<T>::CompareAndSwap(T expected, T x)
{
  return plAtomicUtils::CompareAndSwap(m_value, expected, x);
}

template <typename T>
PLASMA_ALWAYS_INLINE plAtomicInteger<T>::operator T() const
{
  return plAtomicUtils::Read(m_value);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_ALWAYS_INLINE plAtomicBool::plAtomicBool() = default;
PLASMA_ALWAYS_INLINE plAtomicBool::~plAtomicBool() = default;

PLASMA_ALWAYS_INLINE plAtomicBool::plAtomicBool(bool value)
{
  Set(value);
}

PLASMA_ALWAYS_INLINE plAtomicBool::plAtomicBool(const plAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

PLASMA_ALWAYS_INLINE bool plAtomicBool::Set(bool value)
{
  return m_iAtomicInt.Set(value ? 1 : 0) != 0;
}

PLASMA_ALWAYS_INLINE void plAtomicBool::operator=(bool value)
{
  Set(value);
}

PLASMA_ALWAYS_INLINE void plAtomicBool::operator=(const plAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

PLASMA_ALWAYS_INLINE plAtomicBool::operator bool() const
{
  return static_cast<plInt32>(m_iAtomicInt) != 0;
}

PLASMA_ALWAYS_INLINE bool plAtomicBool::TestAndSet(bool bExpected, bool bNewValue)
{
  return m_iAtomicInt.TestAndSet(bExpected ? 1 : 0, bNewValue ? 1 : 0) != 0;
}
