#pragma once

#include <Foundation/Math/Vec3.h>

#include <Core/World/Declarations.h>
#include <Foundation/Types/RefCounted.h>

struct PL_CORE_DLL plCoordinateSystem
{
  PL_DECLARE_POD_TYPE();

  plVec3 m_vForwardDir;
  plVec3 m_vRightDir;
  plVec3 m_vUpDir;
};

class PL_CORE_DLL plCoordinateSystemProvider : public plRefCounted
{
public:
  plCoordinateSystemProvider(const plWorld* pOwnerWorld)
    : m_pOwnerWorld(pOwnerWorld)
  {
  }

  virtual ~plCoordinateSystemProvider() = default;

  virtual void GetCoordinateSystem(const plVec3& vGlobalPosition, plCoordinateSystem& out_coordinateSystem) const = 0;

protected:
  friend class plWorld;

  const plWorld* m_pOwnerWorld;
};

/// \brief Helper class to convert between two plCoordinateSystem spaces.
///
/// All functions will do an identity transform until SetConversion is called to set up
/// the conversion. Afterwards the convert functions can be used to convert between
/// the two systems in both directions.
/// Currently, only uniformly scaled orthogonal coordinate systems are supported.
/// They can however be right handed or left handed.
class PL_CORE_DLL plCoordinateSystemConversion
{
public:
  /// \brief Creates a new conversion that until set up, does identity conversions.
  plCoordinateSystemConversion(); // [tested]

  /// \brief Set up the source and target coordinate systems.
  void SetConversion(const plCoordinateSystem& source, const plCoordinateSystem& target); // [tested]
  /// \brief Returns the equivalent point in the target coordinate system.
  plVec3 ConvertSourcePosition(const plVec3& vPos) const; // [tested]
  /// \brief Returns the equivalent rotation in the target coordinate system.
  plQuat ConvertSourceRotation(const plQuat& qOrientation) const; // [tested]
  /// \brief Returns the equivalent length in the target coordinate system.
  float ConvertSourceLength(float fLength) const; // [tested]

  /// \brief Returns the equivalent point in the source coordinate system.
  plVec3 ConvertTargetPosition(const plVec3& vPos) const; // [tested]
  /// \brief Returns the equivalent rotation in the source coordinate system.
  plQuat ConvertTargetRotation(const plQuat& qOrientation) const; // [tested]
  /// \brief Returns the equivalent length in the source coordinate system.
  float ConvertTargetLength(float fLength) const; // [tested]

private:
  plMat3 m_mSourceToTarget;
  plMat3 m_mTargetToSource;
  float m_fWindingSwap = 1.0f;
  float m_fSourceToTargetScale = 1.0f;
  float m_fTargetToSourceScale = 1.0f;
};
