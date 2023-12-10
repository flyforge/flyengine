#pragma once

PLASMA_ALWAYS_INLINE void plSimdMat4f::Transpose()
{
  _MM_TRANSPOSE4_PS(m_col0.m_v, m_col1.m_v, m_col2.m_v, m_col3.m_v);
}
