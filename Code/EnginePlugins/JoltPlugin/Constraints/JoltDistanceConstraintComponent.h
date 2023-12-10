#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltDistanceConstraintComponentManager = plComponentManager<class plJoltDistanceConstraintComponent, plBlockStorageType::Compact>;

class PLASMA_JOLTPLUGIN_DLL plJoltDistanceConstraintComponent : public plJoltConstraintComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltDistanceConstraintComponent, plJoltConstraintComponent, plJoltDistanceConstraintComponentManager);

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
  // plJoltDistanceConstraintComponent

public:
  plJoltDistanceConstraintComponent();
  ~plJoltDistanceConstraintComponent();

  float GetMinDistance() const { return m_fMinDistance; } // [ property ]
  void SetMinDistance(float value);                       // [ property ]

  float GetMaxDistance() const { return m_fMaxDistance; } // [ property ]
  void SetMaxDistance(float value);                       // [ property ]

  void SetFrequency(float value);                     // [ property ]
  float GetFrequency() const { return m_fFrequency; } // [ property ]

  void SetDamping(float value);                   // [ property ]
  float GetDamping() const { return m_fDamping; } // [ property ]

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

protected:
  float m_fMinDistance = 0.0f;
  float m_fMaxDistance = 1.0f;
  float m_fFrequency = 0.0f;
  float m_fDamping = 0.0f;
};
