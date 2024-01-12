#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltHingeConstraintComponentManager = plComponentManager<class plJoltHingeConstraintComponent, plBlockStorageType::Compact>;

/// \brief Implements a rotational physics constraint.
///
/// Hinge constraints are typically used for doors and wheels. They may either rotate freely
/// or be limited between an upper and lower angle.
/// It is possible to enable a drive to make the hinge rotate at a certain speed, or return to a desired angle.
class PLASMA_JOLTPLUGIN_DLL plJoltHingeConstraintComponent : public plJoltConstraintComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltHingeConstraintComponent, plJoltConstraintComponent, plJoltHingeConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltHingeConstraintComponent

public:
  plJoltHingeConstraintComponent();
  ~plJoltHingeConstraintComponent();

  /// \brief Enables a rotational limit on the hinge.
  void SetLimitMode(plJoltConstraintLimitMode::Enum mode);                     // [ property ]
  plJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  /// \brief Sets how far the hinge may rotate in one direction.
  void SetLowerLimitAngle(plAngle f);                         // [ property ]
  plAngle GetLowerLimitAngle() const { return m_LowerLimit; } // [ property ]

  /// \brief Sets how far the hinge may rotate in the other direction.
  void SetUpperLimitAngle(plAngle f);                         // [ property ]
  plAngle GetUpperLimitAngle() const { return m_UpperLimit; } // [ property ]

  /// \brief Sets how difficult it is to rotate the hinge.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// \brief Enables a drive for the hinge to either constantly rotate or attempt to reach a certain rotation angle.
  void SetDriveMode(plJoltConstraintDriveMode::Enum mode);                     // [ property ]
  plJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  /// \brief Sets the drive target angle or velocity.
  void SetDriveTargetValue(plAngle f);                               // [ property ]
  plAngle GetDriveTargetValue() const { return m_DriveTargetValue; } // [ property ]

  /// \brief Sets how much force the drive may use to reach its target.
  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

protected:
  plEnum<plJoltConstraintLimitMode> m_LimitMode;
  plAngle m_LowerLimit;
  plAngle m_UpperLimit;
  float m_fFriction = 0;
  plEnum<plJoltConstraintDriveMode> m_DriveMode;
  plAngle m_DriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
