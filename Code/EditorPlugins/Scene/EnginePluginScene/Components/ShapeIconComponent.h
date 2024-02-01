#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

using plShapeIconComponentManager = plComponentManager<class plShapeIconComponent, plBlockStorageType::Compact>;

/// \brief This is a dummy component that the editor creates on all 'empty' nodes for the sole purpose to render a shape icon and enable picking.
///
/// Though in the future one could potentially use them for other editor functionality, such as displaying the object name or some other useful text.
class PL_ENGINEPLUGINSCENE_DLL plShapeIconComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plShapeIconComponent, plComponent, plShapeIconComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plShapeIconComponent

public:
  plShapeIconComponent();
  ~plShapeIconComponent();
};
