#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltConeConstraintComponentManager = plComponentManager<class plJoltConeConstraintComponent, plBlockStorageType::Compact>;

class PLASMA_JOLTPLUGIN_DLL plJoltConeConstraintComponent : public plJoltConstraintComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltConeConstraintComponent, plJoltConstraintComponent, plJoltConeConstraintComponentManager);

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
  // plJoltConeConstraintComponent

public:
  plJoltConeConstraintComponent();
  ~plJoltConeConstraintComponent();

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  void SetConeAngle(plAngle f);                        // [ property ]
  plAngle GetConeAngle() const { return m_ConeAngle; } // [ property ]

protected:
  plAngle m_ConeAngle;
};
