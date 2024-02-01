#pragma once

#include <Core/World/ComponentManager.h>

using plCameraShakeComponentManager = plComponentManagerSimple<class plCameraShakeComponent, plComponentUpdateType::WhenSimulating>;

/// \brief This component is used to apply a shaking effect to the game object that it is attached to.
///
/// The shake is applied as a local rotation around the Y and Z axis, assuming the camera is looking along the positive X axis.
/// The component can be attached to the same object as a camera component,
/// but it is usually best to insert a dedicated shake object as a parent of the camera.
///
/// How much shake to apply is controlled through the m_MinShake and m_MaxShake properties.
///
/// The shake values can be modified dynamically to force a shake, but it is more convenient to instead place shake volumes (see plCameraShakeVolumeComponent and derived classes). The camera shake component samples these volumes using its own location and uses the
/// determined strength to fade between its min and max shake amount.
///
/// \see plCameraShakeVolumeComponent
class PL_GAMECOMPONENTS_DLL plCameraShakeComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plCameraShakeComponent, plComponent, plCameraShakeComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plCameraShakeComponent

  /// \brief How much shake to apply as the minimum value, even if no shake volume is found or the shake strength is zero.
  plAngle m_MinShake; // [ property ]

  /// \brief How much shake to apply at shake strength 1.
  plAngle m_MaxShake; // [ property ]

public:
  plCameraShakeComponent();
  ~plCameraShakeComponent();

protected:
  void Update();

  void GenerateKeyframe();
  float GetStrengthAtPosition() const;

  float m_fLastStrength = 0.0f;
  plTime m_ReferenceTime;
  plAngle m_Rotation;
  plQuat m_qPrevTarget = plQuat::MakeIdentity();
  plQuat m_qNextTarget = plQuat::MakeIdentity();
};
