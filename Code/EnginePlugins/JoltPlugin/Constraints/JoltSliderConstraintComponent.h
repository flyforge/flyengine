#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltSliderConstraintComponentManager = plComponentManager<class plJoltSliderConstraintComponent, plBlockStorageType::Compact>;

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


  //////////////////////////////////////////////////////////////////////////
  // plJoltSliderConstraintComponent

public:
  plJoltSliderConstraintComponent();
  ~plJoltSliderConstraintComponent();

  void SetLimitMode(plJoltConstraintLimitMode::Enum mode);                     // [ property ]
  plJoltConstraintLimitMode::Enum GetLimitMode() const { return m_LimitMode; } // [ property ]

  void SetLowerLimitDistance(float f);                                  // [ property ]
  float GetLowerLimitDistance() const { return m_fLowerLimitDistance; } // [ property ]

  void SetUpperLimitDistance(float f);                                  // [ property ]
  float GetUpperLimitDistance() const { return m_fUpperLimitDistance; } // [ property ]

  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  void SetDriveMode(plJoltConstraintDriveMode::Enum mode);                     // [ property ]
  plJoltConstraintDriveMode::Enum GetDriveMode() const { return m_DriveMode; } // [ property ]

  void SetDriveTargetValue(float f);                                // [ property ]
  float GetDriveTargetValue() const { return m_fDriveTargetValue; } // [ property ]

  void SetDriveStrength(float f);                             // [ property ]
  float GetDriveStrength() const { return m_fDriveStrength; } // [ property ]

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

protected:
  plEnum<plJoltConstraintLimitMode> m_LimitMode;
  float m_fLowerLimitDistance = 0;
  float m_fUpperLimitDistance = 0;
  float m_fFriction = 0;

  plEnum<plJoltConstraintDriveMode> m_DriveMode;
  float m_fDriveTargetValue;
  float m_fDriveStrength = 0; // 0 means maximum strength
};
