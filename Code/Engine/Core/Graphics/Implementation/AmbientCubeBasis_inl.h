#pragma once

template <typename T>
PL_ALWAYS_INLINE plAmbientCube<T>::plAmbientCube()
{
  plMemoryUtils::ZeroFillArray(m_Values);
}

template <typename T>
template <typename U>
PL_ALWAYS_INLINE plAmbientCube<T>::plAmbientCube(const plAmbientCube<U>& other)
{
  *this = other;
}

template <typename T>
template <typename U>
PL_FORCE_INLINE void plAmbientCube<T>::operator=(const plAmbientCube<U>& other)
{
  for (plUInt32 i = 0; i < plAmbientCubeBasis::NumDirs; ++i)
  {
    m_Values[i] = other.m_Values[i];
  }
}

template <typename T>
PL_FORCE_INLINE bool plAmbientCube<T>::operator==(const plAmbientCube& other) const
{
  return plMemoryUtils::IsEqual(m_Values, other.m_Values);
}

template <typename T>
PL_ALWAYS_INLINE bool plAmbientCube<T>::operator!=(const plAmbientCube& other) const
{
  return !(*this == other);
}

template <typename T>
void plAmbientCube<T>::AddSample(const plVec3& vDir, const T& value)
{
  m_Values[vDir.x > 0.0f ? 0 : 1] += plMath::Abs(vDir.x) * value;
  m_Values[vDir.y > 0.0f ? 2 : 3] += plMath::Abs(vDir.y) * value;
  m_Values[vDir.z > 0.0f ? 4 : 5] += plMath::Abs(vDir.z) * value;
}

template <typename T>
T plAmbientCube<T>::Evaluate(const plVec3& vNormal) const
{
  plVec3 vNormalSquared = vNormal.CompMul(vNormal);
  return vNormalSquared.x * m_Values[vNormal.x > 0.0f ? 0 : 1] + vNormalSquared.y * m_Values[vNormal.y > 0.0f ? 2 : 3] +
         vNormalSquared.z * m_Values[vNormal.z > 0.0f ? 4 : 5];
}

template <typename T>
plResult plAmbientCube<T>::Serialize(plStreamWriter& inout_stream) const
{
  return inout_stream.WriteArray(m_Values);
}

template <typename T>
plResult plAmbientCube<T>::Deserialize(plStreamReader& inout_stream)
{
  return inout_stream.ReadArray(m_Values);
}
