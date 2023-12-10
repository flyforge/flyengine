#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltFixedConstraintComponentManager = plComponentManager<class plJoltFixedConstraintComponent, plBlockStorageType::Compact>;

class PLASMA_JOLTPLUGIN_DLL plJoltFixedConstraintComponent : public plJoltConstraintComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltFixedConstraintComponent, plJoltConstraintComponent, plJoltFixedConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plJoltFixedConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltFixedConstraintComponent

public:
  plJoltFixedConstraintComponent();
  ~plJoltFixedConstraintComponent();

  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;
};
