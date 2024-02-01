#pragma once

#include <RendererCore/Components/RenderComponent.h>

using plAlwaysVisibleComponentManager = plComponentManager<class plAlwaysVisibleComponent, plBlockStorageType::Compact>;

/// \brief Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class PL_RENDERERCORE_DLL plAlwaysVisibleComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plAlwaysVisibleComponent, plRenderComponent, plAlwaysVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // plAlwaysVisibleComponent

public:
  plAlwaysVisibleComponent();
  ~plAlwaysVisibleComponent();
};
