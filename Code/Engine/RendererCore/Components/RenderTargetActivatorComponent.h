#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

struct plMsgExtractRenderData;

using plRenderTargetComponentManager = plComponentManager<class plRenderTargetActivatorComponent, plBlockStorageType::Compact>;

/// \brief Attach this component to an object that uses a render target for reading, to ensure that the render target gets written to.
///
/// If you build a monitor that displays the output of a security camera in your level, the engine needs to know when it should
/// update the render target that displays the security camera footage, and when it can skip that part to not waste performance.
/// Thus, by default, the engine will not update the render target, as long as this isn't requested.
/// This component implements this request functionality.
///
/// It is a render component, which means that it tracks when it is visible and when visible, it will 'activate' the desired
/// render target, so that it will be updated.
/// By attaching it to an object, like the monitor, it activates the render target whenever the monitor object itself gets rendered.
class PL_RENDERERCORE_DLL plRenderTargetActivatorComponent : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plRenderTargetActivatorComponent, plRenderComponent, plRenderTargetComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent
public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderTargetActivatorComponent

public:
  plRenderTargetActivatorComponent();
  ~plRenderTargetActivatorComponent();

  /// \brief Sets the resource file for the plRenderToTexture2DResource
  void SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;      // [ property ]

  /// \brief Sets the plRenderToTexture2DResource to render activate.
  void SetRenderTarget(const plRenderToTexture2DResourceHandle& hResource);
  plRenderToTexture2DResourceHandle GetRenderTarget() const { return m_hRenderTarget; }

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plRenderToTexture2DResourceHandle m_hRenderTarget;
};
