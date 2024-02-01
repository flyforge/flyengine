#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltFixedConstraintComponentManager = plComponentManager<class plJoltFixedConstraintComponent, plBlockStorageType::Compact>;

/// \brief Implements a fixed physics constraint.
///
/// Actors constrained this way may not move apart, at all.
/// This is mainly useful for adding constraints dynamically, for example to attach a dynamic object to another one once it hits it,
/// or to make it breakable, such that it gets removed when too much force acts on it.
class PL_JOLTPLUGIN_DLL plJoltFixedConstraintComponent : public plJoltConstraintComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltFixedConstraintComponent, plJoltConstraintComponent, plJoltFixedConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plJoltFixedConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltFixedConstraintComponent

public:
  plJoltFixedConstraintComponent();
  ~plJoltFixedConstraintComponent();
};
