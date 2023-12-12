#pragma once

#include <RendererCore/Components/RenderComponent.h>

typedef plComponentManager<class plAlwaysVisibleComponent, plBlockStorageType::Compact> plAlwaysVisibleComponentManager;

/// \brief Attaching this component to a game object makes the renderer consider it always visible, ie. disables culling
class PLASMA_RENDERERCORE_DLL plAlwaysVisibleComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAlwaysVisibleComponent, plRenderComponent, plAlwaysVisibleComponentManager);

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
