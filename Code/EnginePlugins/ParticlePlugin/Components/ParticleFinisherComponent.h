#pragma once

#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

struct plMsgExtractRenderData;

class PLASMA_PARTICLEPLUGIN_DLL plParticleFinisherComponentManager final : public plComponentManager<class plParticleFinisherComponent, plBlockStorageType::Compact>
{
  using SUPER = plComponentManager<class plParticleFinisherComponent, plBlockStorageType::Compact>;

public:
  plParticleFinisherComponentManager(plWorld* pWorld);

  void UpdateBounds();
};

class PLASMA_PARTICLEPLUGIN_DLL plParticleFinisherComponent final : public plRenderComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plParticleFinisherComponent, plRenderComponent, plParticleFinisherComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg) override;


  //////////////////////////////////////////////////////////////////////////
  // plParticleFinisherComponent

public:
  plParticleFinisherComponent();
  ~plParticleFinisherComponent();

  plParticleEffectController m_EffectController;

protected:
  void OnMsgExtractRenderData(plMsgExtractRenderData& msg) const;

  void UpdateBounds();
};
