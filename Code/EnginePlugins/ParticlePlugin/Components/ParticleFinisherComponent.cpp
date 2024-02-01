#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

plParticleFinisherComponentManager::plParticleFinisherComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}

void plParticleFinisherComponentManager::UpdateBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdateBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plParticleFinisherComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plHiddenAttribute,
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE
// clang-format on

plParticleFinisherComponent::plParticleFinisherComponent() = default;
plParticleFinisherComponent::~plParticleFinisherComponent() = default;

void plParticleFinisherComponent::OnDeactivated()
{
  m_EffectController.StopImmediate();

  plRenderComponent::OnDeactivated();
}

plResult plParticleFinisherComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_EffectController.IsAlive())
  {
    m_EffectController.GetBoundingVolume(ref_bounds);
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plParticleFinisherComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  m_EffectController.ExtractRenderData(msg, GetOwner()->GetGlobalTransform());
}

void plParticleFinisherComponent::UpdateBounds()
{
  if (m_EffectController.IsAlive())
  {
    // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
    // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
    GetOwner()->UpdateLocalBounds();
    GetOwner()->UpdateGlobalBounds();
  }
  else
  {
    GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
  }
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleFinisherComponent);

