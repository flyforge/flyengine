#pragma once

/// \brief A 16 bit IEEE float class. Often called "half"
///
/// This class only contains functions to convert between float and float16. It does not support any mathematical operations.
/// It is only intended for conversion, always do all mathematical operations on regular floats (or let the GPU do them on halfs).
class PLASMA_FOUNDATION_DLL plFloat16
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize the value.
  plFloat16() = default;

  /// \brief Create float16 from float.
  plFloat16(float f); // [tested]

  /// \brief Create float16 from float.
  void operator=(float f); // [tested]

  /// \brief Create float16 from raw data.
  void SetRawData(plUInt16 uiData) { m_uiData = uiData; } // [tested]

  /// \brief Returns the raw 16 Bit data.
  plUInt16 GetRawData() const { return m_uiData; } // [tested]

  /// \brief Convert float16 to float.
  operator float() const; // [tested]

  /// \brief Returns true, if both values are identical.
  bool operator==(const plFloat16& c2) { return m_uiData == c2.m_uiData; } // [tested]

  /// \brief Returns true, if both values are not identical.
  bool operator!=(const plFloat16& c2) { return m_uiData != c2.m_uiData; } // [tested]

private:
  /// Raw 16 float data.
  plUInt16 m_uiData;
};

/// \brief A simple helper class to use half-precision floats (plFloat16) as vectors
class PLASMA_FOUNDATION_DLL plFloat16Vec2
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  plFloat16Vec2() = default;
  plFloat16Vec2(const plVec2& vVec);

  void operator=(const plVec2& vVec);
  operator plVec2() const;

  plFloat16 x, y;
};

/// \brief A simple helper class to use half-precision floats (plFloat16) as vectors
class PLASMA_FOUNDATION_DLL plFloat16Vec3
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  plFloat16Vec3() = default;
  plFloat16Vec3(const plVec3& vVec);

  void operator=(const plVec3& vVec);
  operator plVec3() const;

  plFloat16 x, y, z;
};

/// \brief A simple helper class to use half-precision floats (plFloat16) as vectors
class PLASMA_FOUNDATION_DLL plFloat16Vec4
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  PLASMA_DECLARE_POD_TYPE();

  plFloat16Vec4() = default;
  plFloat16Vec4(const plVec4& vVec);

  void operator=(const plVec4& vVec);
  operator plVec4() const;

  plFloat16 x, y, z, w;
};
