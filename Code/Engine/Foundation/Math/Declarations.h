#pragma once

/// \file

#include <Foundation/Basics.h>

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
#  define PL_NAN_ASSERT(obj) (obj)->AssertNotNaN();
#else
#  define PL_NAN_ASSERT(obj)
#endif

#define PL_DECLARE_IF_FLOAT_TYPE template <typename = typename std::enable_if<std::is_floating_point_v<Type> == true>>
#define PL_IMPLEMENT_IF_FLOAT_TYPE template <typename ENABLE_IF_FLOAT>

/// \brief Simple helper union to store ints and floats to modify their bit patterns.
union plIntFloatUnion
{
  constexpr plIntFloatUnion(float fInit)
    : f(fInit)
  {
  }

  constexpr plIntFloatUnion(plUInt32 uiInit)
    : i(uiInit)
  {
  }

  plUInt32 i;
  float f;
};

/// \brief Simple helper union to store ints and doubles to modify their bit patterns.
union plInt64DoubleUnion
{

  constexpr plInt64DoubleUnion(double fInit)
    : f(fInit)
  {
  }
  constexpr plInt64DoubleUnion(plUInt64 uiInit)
    : i(uiInit)
  {
  }

  plUInt64 i;
  double f;
};

/// \brief Enum to describe which memory layout is used to store a matrix in a float array.
///
/// All plMatX classes use column-major format internally. That means they contain one array
/// of, e.g. 16 elements, and the first elements represent the first column, then the second column, etc.
/// So the data is stored column by column and is thus column-major.
/// Some other libraries, such as OpenGL or DirectX require data represented either in column-major
/// or row-major format. plMatrixLayout allows to retrieve the data from an plMatX class in the proper format,
/// and it also allows to pass matrix data as an array back in the plMatX class, and have it converted properly.
/// That means, if you need to pass the content of an plMatX to a function that requires the data in row-major
/// format, you specify that you want to convert the matrix to plMatrixLayout::RowMajor format and you will get
/// the data properly transposed. If a function requires data in column-major format, you specify
/// plMatrixLayout::ColumnMajor and you get it in column-major format (which is simply a memcpy).
struct plMatrixLayout
{
  enum Enum
  {
    RowMajor,   ///< The matrix is stored in row-major format.
    ColumnMajor ///< The matrix is stored in column-major format.
  };
};

/// \brief Describes for which depth range a projection matrix is constructed.
///
/// Different Rendering APIs use different depth ranges.
/// E.g. OpenGL uses -1 for the near plane and +1 for the far plane.
/// DirectX uses 0 for the near plane and 1 for the far plane.
struct plClipSpaceDepthRange
{
  enum Enum
  {
    MinusOneToOne, ///< Near plane at -1, far plane at +1
    ZeroToOne,     ///< Near plane at 0, far plane at 1
  };

  /// \brief Holds the default value for the projection depth range on each platform.
  /// This can be overridden by renderers to ensure the proper range is used when they become active.
  /// On Windows/D3D this is initialized with 'ZeroToOne' by default on all other platforms/OpenGL it is initialized with 'MinusOneToOne' by default.
  PL_FOUNDATION_DLL static Enum Default;
};

/// \brief Specifies whether a projection matrix should flip the result along the Y axis or not.
///
/// Mostly needed to compensate for differing Y texture coordinate conventions. Ie. on some platforms
/// the Y texture coordinate origin is at the lower left and on others on the upper left. To prevent having
/// to modify content to compensate, instead textures are simply flipped along Y on texture load.
/// The same has to be done for all render targets, ie. content has to be rendered upside-down.
///
/// Use plClipSpaceYMode::RenderToTextureDefault when rendering to a texture, to always get the correct
/// projection matrix.
struct plClipSpaceYMode
{
  enum Enum
  {
    Regular, ///< Creates a regular projection matrix
    Flipped, ///< Creates a projection matrix that flips the image on its head. On platforms with different Y texture coordinate
             ///< conventions, this can be used to compensate, by rendering images flipped to render targets.
  };

  /// \brief Holds the platform default value for the clip space Y mode when rendering to a texture.
  /// This can be overridden by renderers to ensure the proper mode is used when they become active.
  /// On Windows/D3D this is initialized with 'Regular' by default on all other platforms/OpenGL it is initialized with 'Flipped' by default.
  PL_FOUNDATION_DLL static Enum RenderToTextureDefault;
};

/// \brief For selecting a left-handed or right-handed convention
struct plHandedness
{
  enum Enum
  {
    LeftHanded,
    RightHanded,
  };

  /// \brief Holds the default handedness value to use. pl uses 'LeftHanded' by default.
  PL_FOUNDATION_DLL static Enum Default /*= plHandedness::LeftHanded*/;
};

// forward declarations
template <typename Type>
class plVec2Template;

using plVec2 = plVec2Template<float>;
using plVec2d = plVec2Template<double>;
using plVec2I32 = plVec2Template<plInt32>;
using plVec2U32 = plVec2Template<plUInt32>;

template <typename Type>
class plVec3Template;

using plVec3 = plVec3Template<float>;
using plVec3d = plVec3Template<double>;
using plVec3I32 = plVec3Template<plInt32>;
using plVec3U32 = plVec3Template<plUInt32>;

template <typename Type>
class plVec4Template;

using plVec4 = plVec4Template<float>;
using plVec4d = plVec4Template<double>;
using plVec4I32 = plVec4Template<plInt32>;
using plVec4I16 = plVec4Template<plInt16>;
using plVec4I8 = plVec4Template<plInt8>;
using plVec4U32 = plVec4Template<plUInt32>;
using plVec4U16 = plVec4Template<plUInt16>;
using plVec4U8 = plVec4Template<plUInt8>;

template <typename Type>
class plMat3Template;

using plMat3 = plMat3Template<float>;
using plMat3d = plMat3Template<double>;

template <typename Type>
class plMat4Template;

using plMat4 = plMat4Template<float>;
using plMat4d = plMat4Template<double>;

template <typename Type>
struct plPlaneTemplate;

using plPlane = plPlaneTemplate<float>;
using plPlaned = plPlaneTemplate<double>;

template <typename Type>
class plQuatTemplate;

using plQuat = plQuatTemplate<float>;
using plQuatd = plQuatTemplate<double>;

template <typename Type>
class plBoundingBoxTemplate;

using plBoundingBox = plBoundingBoxTemplate<float>;
using plBoundingBoxd = plBoundingBoxTemplate<double>;
using plBoundingBoxu32 = plBoundingBoxTemplate<plUInt32>;

template <typename Type>
class plBoundingBoxSphereTemplate;

using plBoundingBoxSphere = plBoundingBoxSphereTemplate<float>;
using plBoundingBoxSphered = plBoundingBoxSphereTemplate<double>;

template <typename Type>
class plBoundingSphereTemplate;

using plBoundingSphere = plBoundingSphereTemplate<float>;
using plBoundingSphered = plBoundingSphereTemplate<double>;

template <plUInt8 DecimalBits>
class plFixedPoint;

class plAngle;

template <typename Type>
class plTransformTemplate;

using plTransform = plTransformTemplate<float>;
using plTransformd = plTransformTemplate<double>;

class plColor;
class plColorLinearUB;
class plColorGammaUB;

class plRandom;

template <typename Type>
class plRectTemplate;

using plRectU32 = plRectTemplate<plUInt32>;
using plRectU16 = plRectTemplate<plUInt16>;
using plRectI32 = plRectTemplate<plInt32>;
using plRectI16 = plRectTemplate<plInt16>;
using plRectFloat = plRectTemplate<float>;
using plRectDouble = plRectTemplate<double>;

class plFrustum;


/// \brief An enum that allows to select on of the six main axis (positive / negative)
struct PL_FOUNDATION_DLL plBasisAxis
{
  using StorageType = plInt8;

  /// \brief An enum that allows to select on of the six main axis (positive / negative)
  enum Enum : plInt8
  {
    PositiveX,
    PositiveY,
    PositiveZ,
    NegativeX,
    NegativeY,
    NegativeZ,

    Default = PositiveX
  };

  /// \brief Returns the vector for the given axis. E.g. (1, 0, 0) or (0, -1, 0), etc.
  static plVec3 GetBasisVector(plBasisAxis::Enum basisAxis);

  /// \brief Computes a matrix representing the transformation. 'Forward' represents the X axis, 'Right' the Y axis and 'Up' the Z axis.
  static plMat3 CalculateTransformationMatrix(plBasisAxis::Enum forwardDir, plBasisAxis::Enum rightDir, plBasisAxis::Enum dir, float fUniformScale = 1.0f, float fScaleX = 1.0f, float fScaleY = 1.0f, float fScaleZ = 1.0f);

  /// \brief Returns a quaternion that rotates from 'identity' to 'axis'
  static plQuat GetBasisRotation(plBasisAxis::Enum identity, plBasisAxis::Enum axis);

  /// \brief Returns a quaternion that rotates from 'PositiveX' to 'axis'
  static plQuat GetBasisRotation_PosX(plBasisAxis::Enum axis);

  /// \brief Returns the axis that is orthogonal to axis1 and axis2. If 'flip' is set, it returns the negated axis.
  ///
  /// If axis1 and axis2 are not orthogonal to each other, the value of axis1 is returned as the result.
  static plBasisAxis::Enum GetOrthogonalAxis(plBasisAxis::Enum axis1, plBasisAxis::Enum axis2, bool bFlip);
};

/// \brief An enum that represents the operator of a comparison
struct PL_FOUNDATION_DLL plComparisonOperator
{
  using StorageType = plUInt8;

  enum Enum
  {
    Equal,
    NotEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,

    Default = Equal
  };

  /// \brief Compares a to b with the given operator. This function only needs the == and < operator for T.
  template <typename T>
  static bool Compare(plComparisonOperator::Enum cmp, const T& a, const T& b); // [tested]
};
