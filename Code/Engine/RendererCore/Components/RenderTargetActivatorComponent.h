#pragma once

#include <Core/World/World.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Textures/Texture2DResource.h>

struct plMsgExtractRenderData;

typedef plComponentManager<class plRenderTargetActivatorComponent, plBlockStorageType::Compact> plRenderTargetComponentManager;

class PLASMA_RENDERERCORE_DLL plRenderTargetActivatorComponent : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plRenderTargetActivatorComponent, plRenderComponent, plRenderTargetComponentManager);

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

  void SetRenderTargetFile(const char* szFile); // [ property ]
  const char* GetRenderTargetFile() const;      // [ property ]

  void SetRenderTarget(const plRenderToTexture2DResourceHandle& hResource);
  plRenderToTexture2DResourceHandle GetRenderTarget() const { return m_hRenderTarget; }

private:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  plRenderToTexture2DResourceHandle m_hRenderTarget;
};
