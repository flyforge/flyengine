#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

class PLASMA_RENDERERCORE_DLL plRenderComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plRenderComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void Deinitialize() override;

  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  plRenderComponent();
  ~plRenderComponent();

  /// \brief Called by plRenderComponent::OnUpdateLocalBounds().
  /// If PLASMA_SUCCESS is returned, \a bounds and \a bAlwaysVisible will be integrated into the plMsgUpdateLocalBounds result,
  /// otherwise the out values are simply ignored.
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) = 0;

  void TriggerLocalBoundsUpdate();

  static plUInt32 GetUniqueIdForRendering(const plComponent* pComponent, plUInt32 uiInnerIndex = 0, plUInt32 uiInnerIndexShift = 24);

  PLASMA_ALWAYS_INLINE plUInt32 GetUniqueIdForRendering(plUInt32 uiInnerIndex = 0, plUInt32 uiInnerIndexShift = 24) const
  {
    return GetUniqueIdForRendering(this, uiInnerIndex, uiInnerIndexShift);
  }

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
};
