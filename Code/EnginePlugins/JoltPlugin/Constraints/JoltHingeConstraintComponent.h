#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltHingeConstraintComponentManager = plComponentManager<class plJoltHingeConstraintComponent, plBlockStorageType::Compact>;

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

  //////////////////////////////////////////////////////////////////////////
  // plJoltHingeConstraintComponent

public:
  plJoltHingeConstraintComponent();
  ~plJoltHingeConstraintComponent();

  void SetLimitMode(plJoltConstraintLimitMode::Enum mode);                     // [ property ]
  plJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  void SetLowerLimitAngle(plAngle f);                         // [ property ]
  plAngle GetLowerLimitAngle() const { return m_LowerLimit; } // [ property ]

  void SetUpperLimitAngle(plAngle f);                         // [ property ]
  plAngle GetUpperLimitAngle() const { return m_UpperLimit; } // [ property ]

  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  void SetDriveMode(plJoltConstraintDriveMode::Enum mode);                     // [ property ]
  plJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  void SetDriveTargetValue(plAngle f);                               // [ property ]
  plAngle GetDriveTargetValue() const { return m_DriveTargetValue; } // [ property ]

  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

protected:
  plEnum<plJoltConstraintLimitMode> m_LimitMode;
  plAngle m_LowerLimit;
  plAngle m_UpperLimit;
  float m_fFriction = 0;
  plEnum<plJoltConstraintDriveMode> m_DriveMode;
  plAngle m_DriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
