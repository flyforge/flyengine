#if 0

#  pragma once

#  include <JoltPlugin/Constraints/JoltConstraintComponent.h>

struct PLASMA_JOLTPLUGIN_DLL plJoltAxis
{
  using StorageType = plUInt8;

  enum Enum
  {
    None = 0,
    X = PLASMA_BIT(0),
    Y = PLASMA_BIT(1),
    Z = PLASMA_BIT(2),
    All = X | Y | Z,
    Default = All
  };

  struct Bits
  {
    StorageType X : 1;
    StorageType Y : 1;
    StorageType Z : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plJoltAxis);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plJoltAxis);

using plJolt6DOFConstraintComponentManager = plComponentManager<class plJolt6DOFConstraintComponent, plBlockStorageType::Compact>;

class PLASMA_JOLTPLUGIN_DLL plJolt6DOFConstraintComponent : public plJoltConstraintComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJolt6DOFConstraintComponent, plJoltConstraintComponent, plJolt6DOFConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;


  //////////////////////////////////////////////////////////////////////////
  // plJolt6DOFConstraintComponent

public:
  plJolt6DOFConstraintComponent();
  ~plJolt6DOFConstraintComponent();

  virtual void ApplySettings() final override;

  void SetFreeLinearAxis(plBitflags<plJoltAxis> flags);                         // [ property ]
  plBitflags<plJoltAxis> GetFreeLinearAxis() const { return m_FreeLinearAxis; } // [ property ]

  void SetFreeAngularAxis(plBitflags<plJoltAxis> flags);                          // [ property ]
  plBitflags<plJoltAxis> GetFreeAngularAxis() const { return m_FreeAngularAxis; } // [ property ]

  void SetLinearLimitMode(plJoltConstraintLimitMode::Enum mode);                           // [ property ]
  plJoltConstraintLimitMode::Enum GetLinearLimitMode() const { return m_LinearLimitMode; } // [ property ]

  void SetLinearRangeX(const plVec2& value);                        // [ property ]
  const plVec2& GetLinearRangeX() const { return m_vLinearRangeX; } // [ property ]
  void SetLinearRangeY(const plVec2& value);                        // [ property ]
  const plVec2& GetLinearRangeY() const { return m_vLinearRangeY; } // [ property ]
  void SetLinearRangeZ(const plVec2& value);                        // [ property ]
  const plVec2& GetLinearRangeZ() const { return m_vLinearRangeZ; } // [ property ]

  void SetLinearStiffness(float f);                               // [ property ]
  float GetLinearStiffness() const { return m_fLinearStiffness; } // [ property ]

  void SetLinearDamping(float f);                             // [ property ]
  float GetLinearDamping() const { return m_fLinearDamping; } // [ property ]

  void SetSwingLimitMode(plJoltConstraintLimitMode::Enum mode);                          // [ property ]
  plJoltConstraintLimitMode::Enum GetSwingLimitMode() const { return m_SwingLimitMode; } // [ property ]

  void SetSwingLimit(plAngle f);                         // [ property ]
  plAngle GetSwingLimit() const { return m_SwingLimit; } // [ property ]

  void SetSwingStiffness(float f);                              // [ property ]
  float GetSwingStiffness() const { return m_fSwingStiffness; } // [ property ]

  void SetSwingDamping(float f);                            // [ property ]
  float GetSwingDamping() const { return m_fSwingDamping; } // [ property ]

  void SetTwistLimitMode(plJoltConstraintLimitMode::Enum mode);                          // [ property ]
  plJoltConstraintLimitMode::Enum GetTwistLimitMode() const { return m_TwistLimitMode; } // [ property ]

  void SetLowerTwistLimit(plAngle f);                              // [ property ]
  plAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  void SetUpperTwistLimit(plAngle f);                              // [ property ]
  plAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  void SetTwistStiffness(float f);                              // [ property ]
  float GetTwistStiffness() const { return m_fTwistStiffness; } // [ property ]

  void SetTwistDamping(float f);                            // [ property ]
  float GetTwistDamping() const { return m_fTwistDamping; } // [ property ]

protected:
  plBitflags<plJoltAxis> m_FreeLinearAxis;

  plEnum<plJoltConstraintLimitMode> m_LinearLimitMode;

  float m_fLinearStiffness = 0.0f;
  float m_fLinearDamping = 0.0f;

  plVec2 m_vLinearRangeX = plVec2::ZeroVector();
  plVec2 m_vLinearRangeY = plVec2::ZeroVector();
  plVec2 m_vLinearRangeZ = plVec2::ZeroVector();

  plBitflags<plJoltAxis> m_FreeAngularAxis;

  plEnum<plJoltConstraintLimitMode> m_SwingLimitMode;
  plAngle m_SwingLimit;

  float m_fSwingStiffness = 0.0f; // [ property ]
  float m_fSwingDamping = 0.0f;   // [ property ]

  plEnum<plJoltConstraintLimitMode> m_TwistLimitMode;
  plAngle m_LowerTwistLimit;
  plAngle m_UpperTwistLimit;

  float m_fTwistStiffness = 0.0f; // [ property ]
  float m_fTwistDamping = 0.0f;   // [ property ]
};

#endif
