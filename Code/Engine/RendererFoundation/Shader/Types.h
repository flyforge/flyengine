#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Transform.h>

/// \brief A wrapper class that converts a plMat3 into the correct data layout for shaders.
class plShaderMat3
{
public:
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plShaderMat3() = default;

  PL_ALWAYS_INLINE plShaderMat3(const plMat3& m) { *this = m; }

  PL_FORCE_INLINE void operator=(const plMat3& m)
  {
    for (plUInt32 c = 0; c < 3; ++c)
    {
      m_Data[c * 4 + 0] = m.Element(c, 0);
      m_Data[c * 4 + 1] = m.Element(c, 1);
      m_Data[c * 4 + 2] = m.Element(c, 2);
      m_Data[c * 4 + 3] = 0.0f;
    }
  }

private:
  float m_Data[12];
};

/// \brief A wrapper class that converts a plTransform into the correct data layout for shaders.
class plShaderTransform
{
public:
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plShaderTransform() = default;

  inline void operator=(const plTransform& t) { *this = t.GetAsMat4(); }

  inline void operator=(const plMat4& t)
  {
    float data[16];
    t.GetAsArray(data, plMatrixLayout::RowMajor);

    for (plUInt32 i = 0; i < 12; ++i)
    {
      m_Data[i] = data[i];
    }
  }

  inline void operator=(const plMat3& t)
  {
    float data[9];
    t.GetAsArray(data, plMatrixLayout::RowMajor);

    m_Data[0] = data[0];
    m_Data[1] = data[1];
    m_Data[2] = data[2];
    m_Data[3] = 0;

    m_Data[4] = data[3];
    m_Data[5] = data[4];
    m_Data[6] = data[5];
    m_Data[7] = 0;

    m_Data[8] = data[6];
    m_Data[9] = data[7];
    m_Data[10] = data[8];
    m_Data[11] = 0;
  }

  inline plMat4 GetAsMat4() const
  {
    plMat4 res;
    res.SetRow(0, reinterpret_cast<const plVec4&>(m_Data[0]));
    res.SetRow(1, reinterpret_cast<const plVec4&>(m_Data[4]));
    res.SetRow(2, reinterpret_cast<const plVec4&>(m_Data[8]));
    res.SetRow(3, plVec4(0, 0, 0, 1));

    return res;
  }

  inline plVec3 GetTranslationVector() const
  {
    return plVec3(m_Data[3], m_Data[7], m_Data[11]);
  }

private:
  float m_Data[12];
};

/// \brief A wrapper class that converts a bool into the correct data layout for shaders.
class plShaderBool
{
public:
  PL_DECLARE_POD_TYPE();

  PL_ALWAYS_INLINE plShaderBool() = default;

  PL_ALWAYS_INLINE plShaderBool(bool b) { m_uiData = b ? 0xFFFFFFFF : 0; }

  PL_ALWAYS_INLINE void operator=(bool b) { m_uiData = b ? 0xFFFFFFFF : 0; }

private:
  plUInt32 m_uiData;
};
