#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/CoordinateSystem.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief Specifies in which mode this camera is configured.
struct PLASMA_CORE_DLL plCameraMode
{
  typedef plInt8 StorageType;

  enum Enum
  {
    None,                 ///< Not initialized
    PerspectiveFixedFovX, ///< Perspective camera, the fov for X is fixed, Y depends on the aspect ratio
    PerspectiveFixedFovY, ///< Perspective camera, the fov for Y is fixed, X depends on the aspect ratio
    OrthoFixedWidth,      ///< Orthographic camera, the width is fixed, the height depends on the aspect ratio
    OrthoFixedHeight,     ///< Orthographic camera, the height is fixed, the width depends on the aspect ratio
    Stereo,               ///< A stereo camera with view/projection matrices provided by an HMD.
    Default = PerspectiveFixedFovY
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plCameraMode);

/// \brief Determines left or right eye of a stereo camera.
///
/// As a general rule, this parameter does not matter for mono-scopic cameras and will always return the same value.
enum class plCameraEye
{
  Left,
  Right,
  // Two eyes should be enough for everyone.
};

/// \brief A camera class that stores the orientation and some basic camera settings.
class PLASMA_CORE_DLL plCamera
{
public:
  plCamera();

  /// \brief Allows to specify a different coordinate system in which the camera input and output coordinates are given.
  ///
  /// The default in z is forward = PositiveX, right = PositiveY, Up = PositiveZ.
  void SetCoordinateSystem(plBasisAxis::Enum forwardAxis, plBasisAxis::Enum rightAxis, plBasisAxis::Enum upAxis);

  /// \brief Allows to specify a full plCoordinateSystemProvider to determine forward/right/up vectors for camera movement
  void SetCoordinateSystem(const plSharedPtr<plCoordinateSystemProvider>& provider);

  /// \brief Returns the position of the camera that should be used for rendering etc.
  plVec3 GetPosition(plCameraEye eye = plCameraEye::Left) const;

  /// \brief Returns the forwards vector that should be used for rendering etc.
  plVec3 GetDirForwards(plCameraEye eye = plCameraEye::Left) const;

  /// \brief Returns the up vector that should be used for rendering etc.
  plVec3 GetDirUp(plCameraEye eye = plCameraEye::Left) const;

  /// \brief Returns the right vector that should be used for rendering etc.
  plVec3 GetDirRight(plCameraEye eye = plCameraEye::Left) const;

  /// \brief Returns the horizontal FOV.
  ///
  /// Works only with plCameraMode::PerspectiveFixedFovX and plCameraMode::PerspectiveFixedFovY
  plAngle GetFovX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical FOV.
  ///
  /// Works only with plCameraMode::PerspectiveFixedFovX and plCameraMode::PerspectiveFixedFovY
  plAngle GetFovY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the horizontal dimension for an orthographic view.
  ///
  /// Works only with plCameraMode::OrthoFixedWidth and plCameraMode::OrthoFixedWidth
  float GetDimensionX(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the vertical dimension for an orthographic view.
  ///
  /// Works only with plCameraMode::OrthoFixedWidth and plCameraMode::OrthoFixedWidth
  float GetDimensionY(float fAspectRatioWidthDivHeight) const;

  /// \brief Returns the average camera position.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetPosition()
  plVec3 GetCenterPosition() const;

  /// \brief Returns the average forwards vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirForwards()
  plVec3 GetCenterDirForwards() const;

  /// \brief Returns the average up vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirUp()
  plVec3 GetCenterDirUp() const;

  /// \brief Returns the average right vector.
  ///
  /// For all cameras execpt Stereo cameras this is identical to GetDirRight()
  plVec3 GetCenterDirRight() const;

  /// \brief Returns the near plane distance that was passed to SetCameraProjectionAndMode().
  float GetNearPlane() const;

  /// \brief Returns the far plane distance that was passed to SetCameraProjectionAndMode().
  float GetFarPlane() const;

  /// \brief Specifies the mode and the projection settings that this camera uses.
  ///
  /// \param fFovOrDim
  ///   Fov X/Y in degree or width/height (depending on Mode).
  void SetCameraMode(plCameraMode::Enum Mode, float fFovOrDim, float fNearPlane, float fFarPlane);

  /// Sets the camera mode to stereo and specifies projection matrices directly.
  ///
  /// \param fAspectRatio
  ///   These stereo projection matrices will only be returned by getProjectionMatrix for the given aspectRatio.
  void SetStereoProjection(const plMat4& mProjectionLeftEye, const plMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight);

  /// \brief Returns the fFovOrDim parameter that was passed to SetCameraProjectionAndMode().
  float GetFovOrDim() const;

  /// \brief Returns the current camera mode.
  plCameraMode::Enum GetCameraMode() const;

  bool IsPerspective() const;

  bool IsOrthographic() const;

  /// \brief Whether this is a stereoscopic camera.
  bool IsStereoscopic() const;

  /// \brief Sets the view matrix directly.
  ///
  /// Works with all camera types. Position- and direction- getter/setter will work as usual.
  void SetViewMatrix(const plMat4& mLookAtMatrix, plCameraEye eye = plCameraEye::Left);

  /// \brief Repositions the camera such that it looks at the given target position.
  ///
  /// Not supported for stereo cameras.
  void LookAt(const plVec3& vCameraPos, const plVec3& vTargetPos, const plVec3& vUp);

  /// \brief Moves the camera in its local space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveLocally(float fForward, float fRight, float fUp);

  /// \brief Moves the camera in global space along the forward/right/up directions of the coordinate system.
  ///
  /// Not supported for stereo cameras.
  void MoveGlobally(float fForward, float fRight, float fUp);

  /// \brief Rotates the camera around the forward, right and up axis in its own local space.
  ///
  /// Rotate around \a rightAxis for looking up/down. \forwardAxis is roll. For turning left/right use RotateGlobally().
  /// Not supported for stereo cameras.
  void RotateLocally(plAngle forwardAxis, plAngle rightAxis, plAngle upAxis);

  /// \brief Rotates the camera around the forward, right and up axis of the coordinate system in global space.
  ///
  /// Rotate around Z for turning the camera left/right.
  /// Not supported for stereo cameras.
  void RotateGlobally(plAngle forwardAxis, plAngle rightAxis, plAngle upAxis);

  /// \brief Returns the view matrix for the given eye.
  ///
  /// \note The view matrix is given in OpenGL convention.
  const plMat4& GetViewMatrix(plCameraEye eye = plCameraEye::Left) const;

  /// \brief Calculates the projection matrix from the current camera properties and stores it in out_projectionMatrix.
  ///
  /// If the camera is stereo and the given aspect ratio is close to the aspect ratio passed in SetStereoProjection,
  /// the matrix set in SetStereoProjection will be used.
  void GetProjectionMatrix(float fAspectRatioWidthDivHeight, plMat4& out_projectionMatrix, plCameraEye eye = plCameraEye::Left,
    plClipSpaceDepthRange::Enum depthRange = plClipSpaceDepthRange::Default) const;

  float GetShutterSpeed() const;

  void SetShutterSpeed(float fShutterSpeed);

  float GetExposure() const;

  void SetExposure(float fExposure);

  float GetAperture() const;

  void SetAperture(float fAperture);

  float GetISO() const;

  void SetISO(float fISO);

  float GetFocusDistance() const;

  void SetFocusDistance(float fFocusDistance);

  /// \brief Returns a counter that is increased every time the camera settings are modified.
  ///
  /// The camera settings are used to compute the projection matrix. This counter can be used to determine whether the projection matrix
  /// has changed and thus whether cached values need to be updated.
  plUInt32 GetSettingsModificationCounter() const { return m_uiSettingsModificationCounter; }

  /// \brief Returns a counter that is increased every time the camera orientation is modified.
  ///
  /// The camera orientation is used to compute the view matrix. This counter can be used to determine whether the view matrix
  /// has changed and thus whether cached values need to be updated.
  plUInt32 GetOrientationModificationCounter() const { return m_uiOrientationModificationCounter; }

private:
  /// \brief This function is called whenever the camera position or rotation changed.
  void CameraOrientationChanged(bool bPosition, bool bRotation) { ++m_uiOrientationModificationCounter; }

  /// \brief This function is called when the camera mode or projection changes (e.g. SetCameraProjectionAndMode was called).
  void CameraSettingsChanged();

  /// \brief This function is called by RotateLocally() and RotateGlobally() BEFORE the values are applied,
  /// and allows to adjust them (e.g. for limiting how far the camera can rotate).
  void ClampRotationAngles(bool bLocalSpace, plAngle& forwardAxis, plAngle& rightAxis, plAngle& upAxis);

  plVec3 InternalGetPosition(plCameraEye eye = plCameraEye::Left) const;
  plVec3 InternalGetDirForwards(plCameraEye eye = plCameraEye::Left) const;
  plVec3 InternalGetDirUp(plCameraEye eye = plCameraEye::Left) const;
  plVec3 InternalGetDirRight(plCameraEye eye = plCameraEye::Left) const;

  float m_fNearPlane = 0.1f;
  float m_fFarPlane = 1000.0f;

  plCameraMode::Enum m_Mode = plCameraMode::None;

  float m_fFovOrDim = 90.0f;

  float m_fShutterSpeed = 0.005f;
  float m_fExposure = 1.0f;
  float m_fAperture = 16.0f;
  float m_fISO = 100.0f;
  float m_fFocusDistance = 1.0f;

  plVec3 m_vCameraPosition[2];
  plMat4 m_mViewMatrix[2];

  /// If the camera mode is stereo and the aspect ratio given in getProjectio is close to this value, one of the stereo projection matrices
  /// is returned.
  float m_fAspectOfPrecomputedStereoProjection = -1.0;
  plMat4 m_mStereoProjectionMatrix[2];

  plUInt32 m_uiSettingsModificationCounter = 0;
  plUInt32 m_uiOrientationModificationCounter = 0;

  plSharedPtr<plCoordinateSystemProvider> m_pCoordinateSystem;

  plVec3 MapExternalToInternal(const plVec3& v) const;
  plVec3 MapInternalToExternal(const plVec3& v) const;
};


#include <Core/Graphics/Implementation/Camera_inl.h>
