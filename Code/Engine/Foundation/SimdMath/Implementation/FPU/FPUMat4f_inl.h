#pragma once

PLASMA_ALWAYS_INLINE void plSimdMat4f::Transpose()
{
  plMath::Swap(m_col0.m_v.y, m_col1.m_v.x);
  plMath::Swap(m_col0.m_v.z, m_col2.m_v.x);
  plMath::Swap(m_col0.m_v.w, m_col3.m_v.x);
  plMath::Swap(m_col1.m_v.z, m_col2.m_v.y);
  plMath::Swap(m_col1.m_v.w, m_col3.m_v.y);
  plMath::Swap(m_col2.m_v.w, m_col3.m_v.z);
}
