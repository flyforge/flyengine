#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltSliderConstraintComponentManager = plComponentManager<class plJoltSliderConstraintComponent, plBlockStorageType::Compact>;

/// \brief Implements a sliding physics constraint.
///
/// The child actor may move along the parent actor along the positive X axis of the constraint.
/// Usually lower and upper limits are used to prevent infinite movement.
class PLASMA_JOLTPLUGIN_DLL plJoltSliderConstraintComponent : public plJoltConstraintComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltSliderConstraintComponent, plJoltConstraintComponent, plJoltSliderConstraintComponentManager);

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
  // plJoltSliderConstraintComponent

public:
  plJoltSliderConstraintComponent();
  ~plJoltSliderConstraintComponent();

  /// \brief Enables a translational limit on the slider.
  void SetLimitMode(plJoltConstraintLimitMode::Enum mode);                     // [ property ]
  plJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  /// \brief Sets how far child actor may move in one direction.
  void SetLowerLimitDistance(float f);                                  // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  /// \brief Sets how far child actor may move in the other direction.
  void SetUpperLimitDistance(float f);                                  // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  /// \brief Sets how difficult it is to move the child actor.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// \brief Enables a drive for the slider to either constantly move or attempt to reach a certain position.
  void SetDriveMode(plJoltConstraintDriveMode::Enum mode);                     // [ property ]
  plJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  /// \brief Sets the drive target position or velocity.
  void SetDriveTargetValue(float f);                                // [ property ]
  float GetDriveTargetValue() const { return m_fDriveTargetValue; } // [ property ]

  /// \brief Sets how much force the drive may use to reach its target.
  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

protected:
  plEnum<plJoltConstraintLimitMode> m_LimitMode;
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fFriction = 0;

  plEnum<plJoltConstraintDriveMode> m_DriveMode;
  float m_fDriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
