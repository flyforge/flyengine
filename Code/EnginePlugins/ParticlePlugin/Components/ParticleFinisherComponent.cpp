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
PLASMA_BEGIN_COMPONENT_TYPE(plParticleFinisherComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plHiddenAttribute,
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plParticleFinisherComponent::plParticleFinisherComponent() = default;
plParticleFinisherComponent::~plParticleFinisherComponent() = default;

void plParticleFinisherComponent::OnDeactivated()
{
  m_EffectController.StopImmediate();

  plRenderComponent::OnDeactivated();
}

plResult plParticleFinisherComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (m_EffectController.IsAlive())
  {
    m_EffectController.GetBoundingVolume(bounds);
    return PLASMA_SUCCESS;
  }

  return PLASMA_FAILURE;
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
