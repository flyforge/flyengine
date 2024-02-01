#pragma once

#include <Core/World/World.h>
#include <ParticlePlugin/Effect/ParticleEffectController.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/ParticlePluginDLL.h>
#include <RendererCore/Components/RenderComponent.h>

struct plMsgExtractRenderData;

class PL_PARTICLEPLUGIN_DLL plParticleFinisherComponentManager final : public plComponentManager<class plParticleFinisherComponent, plBlockStorageType::Compact>
{
  using SUPER = plComponentManager<class plParticleFinisherComponent, plBlockStorageType::Compact>;

public:
  plParticleFinisherComponentManager(plWorld* pWorld);

  void UpdateBounds();
};

/// \brief Automatically created by the particle system to finish playing a particle effect.
///
/// This is needed to play a particle effect to the end, when a game object with a particle effect on it gets deleted.
/// This component should never be instantiated manually.
class PL_PARTICLEPLUGIN_DLL plParticleFinisherComponent final : public plRenderComponent
{
  PL_DECLARE_COMPONENT_TYPE(plParticleFinisherComponent, plRenderComponent, plParticleFinisherComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) override;

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
