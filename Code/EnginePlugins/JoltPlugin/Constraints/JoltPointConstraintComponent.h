#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltPointConstraintComponentManager = plComponentManager<class plJoltPointConstraintComponent, plBlockStorageType::Compact>;

/// \brief Implements a physics constraint that allows rotation around one point.
///
/// The joined actors can freely rotate around the constraint position.
class PL_JOLTPLUGIN_DLL plJoltPointConstraintComponent : public plJoltConstraintComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltPointConstraintComponent, plJoltConstraintComponent, plJoltPointConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltConstraintComponent

protected:
  virtual void ApplySettings() override;
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual bool ExceededBreakingPoint() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltPointConstraintComponent

public:
  plJoltPointConstraintComponent();
  ~plJoltPointConstraintComponent();
};
