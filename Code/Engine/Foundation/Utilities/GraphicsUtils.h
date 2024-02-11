#pragma once

#include <Foundation/Math/Mat4.h>

namespace plGraphicsUtils
{
  /// \brief Projects the given point from 3D world space into screen space, if possible.
  ///
  /// \param ModelViewProjection
  ///   The Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see plClipSpaceDepthRange
  ///
  /// Returns PL_FAILURE, if the point could not be projected into screen space.
  /// \note The function reports PL_SUCCESS, when the point could be projected, however, that does not mean that the point actually lies
  /// within the viewport, it might still be outside the viewport.
  ///
  /// out_vScreenPos.z is the depth of the point in [0;1] range. The z value is always 'normalized' to this range
  /// (as long as the DepthRange parameter is correct), to make it easier to make subsequent code platform independent.
  PL_FOUNDATION_DLL plResult ConvertWorldPosToScreenPos(const plMat4& mModelViewProjection, const plUInt32 uiViewportX, const plUInt32 uiViewportY,
    const plUInt32 uiViewportWidth, const plUInt32 uiViewportHeight, const plVec3& vPoint, plVec3& out_vScreenPos,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default); // [tested]

  /// \brief Takes the screen space position (including depth in [0;1] range) and converts it into a world space position.
  ///
  /// \param InverseModelViewProjection
  ///   The inverse of the Model-View-Projection matrix that is used by the camera.
  /// \param DepthRange
  ///   The depth range that is used by this projection matrix. \see plClipSpaceDepthRange
  ///
  /// Returns PL_FAILURE when the screen coordinate could not be converted to a world position,
  /// which should generally not be possible as long as the coordinate is actually inside the viewport.
  ///
  /// Optionally this function also computes the direction vector through the world space position, that should be used for picking
  /// operations. Note that for perspective cameras this is the same as the direction from the camera position to the computed point,
  /// but for orthographic cameras it is not (it's simply the forward vector of the camera).
  /// This function handles both cases properly.
  ///
  /// The z value of vScreenPos is always expected to be in [0; 1] range (meaning 0 is at the near plane, 1 at the far plane),
  /// even on platforms that use [-1; +1] range for clip-space z values. The DepthRange parameter needs to be correct to handle this case
  /// properly.
  PL_FOUNDATION_DLL plResult ConvertScreenPosToWorldPos(const plMat4& mInverseModelViewProjection, const plUInt32 uiViewportX,
    const plUInt32 uiViewportY, const plUInt32 uiViewportWidth, const plUInt32 uiViewportHeight, const plVec3& vScreenPos, plVec3& out_vPoint,
    plVec3* out_pDirection = nullptr,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default); // [tested]

  /// \brief A double-precision version of ConvertScreenPosToWorldPos()
  PL_FOUNDATION_DLL plResult ConvertScreenPosToWorldPos(const plMat4d& mInverseModelViewProjection, const plUInt32 uiViewportX,
    const plUInt32 uiViewportY, const plUInt32 uiViewportWidth, const plUInt32 uiViewportHeight, const plVec3& vScreenPos, plVec3& out_vPoint,
    plVec3* out_pDirection = nullptr,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default); // [tested]

  /// \brief Checks whether the given transformation matrix would change the winding order of a triangle's vertices and thus requires that
  /// the vertex order gets reversed to compensate.
  PL_FOUNDATION_DLL bool IsTriangleFlipRequired(const plMat3& mTransformation);

  /// \brief Converts a projection or view-projection matrix from one depth-range convention to another
  PL_FOUNDATION_DLL void ConvertProjectionMatrixDepthRange(
    plMat4& inout_mMatrix, plClipSpaceDepthRange::Enum srcDepthRange, plClipSpaceDepthRange::Enum dstDepthRange); // [tested]

  /// \brief Retrieves the horizontal and vertical field-of-view angles from the perspective matrix.
  ///
  /// \note If an orthographic projection matrix is passed in, the returned angle values will be zero.
  PL_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const plMat4& mProjectionMatrix, plAngle& out_fovX, plAngle& out_fovY); // [tested]

  /// \brief Extracts the field of view angles from a perspective matrix.
  /// \param ProjectionMatrix Perspective projection matrix to be decomposed.
  /// \param out_fFovLeft Left angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovRight Right angle of the frustum.
  /// \param out_fFovBottom Bottom angle of the frustum. Negative in symmetric projection.
  /// \param out_fFovTop Top angle of the frustum.
  /// \param yRange The Y range used to construct the perspective matrix.
  PL_FOUNDATION_DLL void ExtractPerspectiveMatrixFieldOfView(const plMat4& mProjectionMatrix, plAngle& out_fovLeft, plAngle& out_fovRight, plAngle& out_fovBottom, plAngle& out_fovTop, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular); // [tested]

  /// \brief Extracts the field of view distances on the near plane from a perspective matrix.
  ///
  /// Convenience function that also extracts near / far values and returns the distances on the near plane to be the inverse of plGraphicsUtils::CreatePerspectiveProjectionMatrix.
  /// \sa plGraphicsUtils::CreatePerspectiveProjectionMatrix
  PL_FOUNDATION_DLL plResult ExtractPerspectiveMatrixFieldOfView(const plMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular); // [tested]

  /// \brief Computes the distances of the near and far clip planes from the given perspective projection matrix.
  ///
  /// Returns PL_FAILURE when one of the values could not be computed, because it would result in a "division by zero".
  PL_FOUNDATION_DLL plResult ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const plMat4& mProjectionMatrix,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default); // [tested]


  enum class FrustumPlaneInterpolation
  {
    LeftToRight,
    BottomToTop,
    NearToFar,
  };

  /// \brief Computes an interpolated frustum plane by using linear interpolation in normalized clip space.
  ///
  /// Along left/right, up/down this makes it easy to create a regular grid of planes.
  /// Along near/far creating planes at regular intervals will result in planes in world-space that represent
  /// the same amount of depth-precision.
  ///
  /// \param dir Specifies which planes to interpolate.
  /// \param fLerpFactor The interpolation coefficient (usually in the interval [0;1]).
  PL_FOUNDATION_DLL plPlane ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation dir, float fLerpFactor, const plMat4& mProjectionMatrix,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default); // [tested]

  /// \brief Creates a perspective projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  PL_FOUNDATION_DLL plMat4 CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  PL_FOUNDATION_DLL plMat4 CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewX    Horizontal field of view.
  PL_FOUNDATION_DLL plMat4 CreatePerspectiveProjectionMatrixFromFovX(plAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Creates a perspective projection matrix.
  /// \param fFieldOfViewY    Vertical field of view.
  PL_FOUNDATION_DLL plMat4 CreatePerspectiveProjectionMatrixFromFovY(plAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ,
    float fFarZ, plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix with Left = -fViewWidth/2, Right = +fViewWidth/2, Bottom = -fViewHeight/2, Top =
  /// +fViewHeight/2.
  PL_FOUNDATION_DLL plMat4 CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Creates an orthographic projection matrix.
  PL_FOUNDATION_DLL plMat4 CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default, plClipSpaceYMode::Enum range = plClipSpaceYMode::Regular,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Returns a look-at matrix (only direction, no translation).
  ///
  /// Since this only creates a rotation matrix, vTarget can be interpreted both as a position or a direction.
  PL_FOUNDATION_DLL plMat3 CreateLookAtViewMatrix(
    const plVec3& vTarget, const plVec3& vUpDir, plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  PL_FOUNDATION_DLL plMat3 CreateInverseLookAtViewMatrix(
    const plVec3& vTarget, const plVec3& vUpDir, plHandedness::Enum handedness = plHandedness::Default); // [tested]


  /// \brief Returns a look-at matrix with both rotation and translation
  PL_FOUNDATION_DLL plMat4 CreateLookAtViewMatrix(const plVec3& vEyePos, const plVec3& vLookAtPos, const plVec3& vUpDir,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Same as CreateLookAtViewMatrix() but returns the inverse matrix
  PL_FOUNDATION_DLL plMat4 CreateInverseLookAtViewMatrix(const plVec3& vEyePos, const plVec3& vLookAtPos, const plVec3& vUpDir,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Creates a view matrix from the given camera vectors.
  ///
  /// The vectors are put into the appropriate matrix rows and depending on the handedness negated where necessary.
  PL_FOUNDATION_DLL plMat4 CreateViewMatrix(const plVec3& vPosition, const plVec3& vForwardDir, const plVec3& vRightDir, const plVec3& vUpDir,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Similar to CreateViewMatrix() but creates the inverse matrix.
  PL_FOUNDATION_DLL plMat4 CreateInverseViewMatrix(const plVec3& vPosition, const plVec3& vForwardDir, const plVec3& vRightDir, const plVec3& vUpDir,
    plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Extracts the forward, right and up dir and camera position from the given view matrix.
  ///
  /// The handedness should be the same as used in CreateViewMatrix() or CreateLookAtViewMatrix().
  PL_FOUNDATION_DLL void DecomposeViewMatrix(plVec3& out_vPosition, plVec3& out_vForwardDir, plVec3& out_vRightDir, plVec3& out_vUpDir,
    const plMat4& mViewMatrix, plHandedness::Enum handedness = plHandedness::Default); // [tested]

  /// \brief Computes the barycentric coordinates of a point in a 3D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns PL_FAILURE.
  PL_FOUNDATION_DLL plResult ComputeBarycentricCoordinates(plVec3& out_vCoordinates, const plVec3& v0, const plVec3& v1, const plVec3& v2, const plVec3& vPos);

  /// \brief Computes the barycentric coordinates of a point in a 2D triangle.
  ///
  /// \return If the triangle is degenerate (all points on a line, or two points identical), the function returns PL_FAILURE.
  PL_FOUNDATION_DLL plResult ComputeBarycentricCoordinates(plVec3& out_vCoordinates, const plVec2& v0, const plVec2& v1, const plVec2& v2, const plVec2& vPos);

   /// \brief Returns a coverage value of how much space a sphere at a given location would take up on screen using a perspective projection.
  ///
  /// The coverage value is close to 0 for very small or far away spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction, it only depends on distance and the camera's field-of-view.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera FOV angle is used for the calculation, pass in either the horizontal or vertical FOV angle,
  /// depending on what is most relevant to you.
  /// Typically the 'fixed' angle is used (usually the vertical one) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(const plBoundingSphere& sphere, const plVec3& vCameraPosition, plAngle perspectiveCameraFov)
  {
    const float fDist = (sphere.m_vCenter - vCameraPosition).GetLength();
    const float fHalfHeight = plMath::Tan(perspectiveCameraFov * 0.5f) * fDist;
    return sphere.m_fRadius / fHalfHeight;
  }

  /// \brief Returns a coverage value of how much space a sphere of a given size would take up on screen using an orthographic projection.
  ///
  /// The coverage value is close to 0 for very small spheres and approaches 1 when the projected sphere would take up the entire screen.
  /// The calculation is resolution independent and also doesn't take into account whether the sphere is inside the view frustum at all.
  /// Thus the value doesn't change depending on camera view direction. In orthographic projections even the distance to the camera is irrelevant,
  /// only the dimensions of the ortho camera are needed.
  /// Values (much) larger than 1 are possible.
  ///
  /// \note Only one camera dimension is used for the calculation, pass in either the X or Y dimension, depending on what is most relevant to you.
  /// Typically the 'fixed' dimension is used (usually Y) since the other one depends on the window size.
  inline float CalculateSphereScreenCoverage(float fSphereRadius, float fOrthoCameraDimensions)
  {
    const float fHalfHeight = fOrthoCameraDimensions * 0.5f;
    return fSphereRadius / fHalfHeight;
  }

} // namespace plGraphicsUtils
