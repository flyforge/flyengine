#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Quat.h>
#include <Foundation/Math/Transform.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Math/Vec4.h>

// plVec2Template

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plVec2Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(plVec2Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plVec2Template<Type>& ref_vValue)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(plVec2Template<Type>)) == sizeof(plVec2Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plVec2Template<Type>* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plVec2Template<Type>) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plVec2Template<Type>* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plVec2Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plVec3Template

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plVec3Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(plVec3Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plVec3Template<Type>& ref_vValue)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(plVec3Template<Type>)) == sizeof(plVec3Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plVec3Template<Type>* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plVec3Template<Type>) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plVec3Template<Type>* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plVec3Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plVec4Template

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plVec4Template<Type>& vValue)
{
  inout_stream.WriteBytes(&vValue, sizeof(plVec4Template<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plVec4Template<Type>& ref_vValue)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&ref_vValue, sizeof(plVec4Template<Type>)) == sizeof(plVec4Template<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plVec4Template<Type>* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plVec4Template<Type>) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plVec4Template<Type>* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plVec4Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plMat3Template

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plMat3Template<Type>& mValue)
{
  inout_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 9).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plMat3Template<Type>& ref_mValue)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 9) == sizeof(Type) * 9, "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plMat3Template<Type>* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plMat3Template<Type>) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plMat3Template<Type>* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plMat3Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plMat4Template

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plMat4Template<Type>& mValue)
{
  inout_stream.WriteBytes(mValue.m_fElementsCM, sizeof(Type) * 16).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plMat4Template<Type>& ref_mValue)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(ref_mValue.m_fElementsCM, sizeof(Type) * 16) == sizeof(Type) * 16, "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plMat4Template<Type>* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plMat4Template<Type>) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plMat4Template<Type>* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plMat4Template<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plTransformTemplate

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plTransformTemplate<Type>& value)
{
  inout_stream << value.m_qRotation;
  inout_stream << value.m_vPosition;
  inout_stream << value.m_vScale;

  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plTransformTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_qRotation;
  inout_stream >> out_value.m_vPosition;
  inout_stream >> out_value.m_vScale;

  return inout_stream;
}

// plPlaneTemplate

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plPlaneTemplate<Type>& value)
{
  inout_stream.WriteBytes(&value, sizeof(plPlaneTemplate<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plPlaneTemplate<Type>& out_value)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&out_value, sizeof(plPlaneTemplate<Type>)) == sizeof(plPlaneTemplate<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plPlaneTemplate<Type>* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plPlaneTemplate<Type>) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plPlaneTemplate<Type>* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plPlaneTemplate<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plQuatTemplate

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plQuatTemplate<Type>& qValue)
{
  inout_stream.WriteBytes(&qValue, sizeof(plQuatTemplate<Type>)).AssertSuccess();
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plQuatTemplate<Type>& ref_qValue)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&ref_qValue, sizeof(plQuatTemplate<Type>)) == sizeof(plQuatTemplate<Type>), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plQuatTemplate<Type>* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plQuatTemplate<Type>) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plQuatTemplate<Type>* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plQuatTemplate<Type>) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plBoundingBoxTemplate

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plBoundingBoxTemplate<Type>& value)
{
  inout_stream << value.m_vMax;
  inout_stream << value.m_vMin;
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plBoundingBoxTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vMax;
  inout_stream >> out_value.m_vMin;
  return inout_stream;
}

// plBoundingSphereTemplate

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plBoundingSphereTemplate<Type>& value)
{
  inout_stream << value.m_vCenter;
  inout_stream << value.m_fRadius;
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plBoundingSphereTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vCenter;
  inout_stream >> out_value.m_fRadius;
  return inout_stream;
}

// plBoundingBoxSphereTemplate

template <typename Type>
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plBoundingBoxSphereTemplate<Type>& value)
{
  inout_stream << value.m_vCenter;
  inout_stream << value.m_fSphereRadius;
  inout_stream << value.m_vBoxHalfExtends;
  return inout_stream;
}

template <typename Type>
inline plStreamReader& operator>>(plStreamReader& inout_stream, plBoundingBoxSphereTemplate<Type>& out_value)
{
  inout_stream >> out_value.m_vCenter;
  inout_stream >> out_value.m_fSphereRadius;
  inout_stream >> out_value.m_vBoxHalfExtends;
  return inout_stream;
}

// plColor
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plColor& value)
{
  inout_stream.WriteBytes(&value, sizeof(plColor)).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plColor& ref_value)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(plColor)) == sizeof(plColor), "End of stream reached.");
  return inout_stream;
}

inline plResult SerializeArray(plStreamWriter& inout_stream, const plColor* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plColor) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plColor* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plColor) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plColorGammaUB
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plColorGammaUB& value)
{
  inout_stream.WriteBytes(&value, sizeof(plColorGammaUB)).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plColorGammaUB& ref_value)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(plColorGammaUB)) == sizeof(plColorGammaUB), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plColorGammaUB* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plColorGammaUB) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plColorGammaUB* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plColorGammaUB) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plAngle
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plAngle& value)
{
  inout_stream << value.GetRadian();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plAngle& out_value)
{
  float fRadian;
  inout_stream >> fRadian;
  out_value.SetRadian(fRadian);
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plAngle* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plAngle) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plAngle* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plAngle) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}


// plColor8Unorm
inline plStreamWriter& operator<<(plStreamWriter& inout_stream, const plColorLinearUB& value)
{
  inout_stream.WriteBytes(&value, sizeof(plColorLinearUB)).AssertSuccess();
  return inout_stream;
}

inline plStreamReader& operator>>(plStreamReader& inout_stream, plColorLinearUB& ref_value)
{
  PLASMA_VERIFY(inout_stream.ReadBytes(&ref_value, sizeof(plColorLinearUB)) == sizeof(plColorLinearUB), "End of stream reached.");
  return inout_stream;
}

template <typename Type>
plResult SerializeArray(plStreamWriter& inout_stream, const plColorLinearUB* pArray, plUInt64 uiCount)
{
  return inout_stream.WriteBytes(pArray, sizeof(plColorLinearUB) * uiCount);
}

template <typename Type>
plResult DeserializeArray(plStreamReader& inout_stream, plColorLinearUB* pArray, plUInt64 uiCount)
{
  const plUInt64 uiNumBytes = sizeof(plColorLinearUB) * uiCount;
  if (inout_stream.ReadBytes(pArray, uiNumBytes) == uiNumBytes)
    return PLASMA_SUCCESS;

  return PLASMA_FAILURE;
}
