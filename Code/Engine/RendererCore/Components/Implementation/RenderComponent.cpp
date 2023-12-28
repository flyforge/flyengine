#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plRenderComponent, 1)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plRenderComponent::plRenderComponent() = default;
plRenderComponent::~plRenderComponent() = default;

void plRenderComponent::Deinitialize()
{
  plRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void plRenderComponent::OnActivated()
{
  TriggerLocalBoundsUpdate();
}

void plRenderComponent::OnDeactivated()
{
  // Can't call TriggerLocalBoundsUpdate because it checks whether we are active, which is not the case anymore.
  GetOwner()->UpdateLocalBounds();
}

void plRenderComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  plBoundingBoxSphere bounds;
  bounds.SetInvalid();

  bool bAlwaysVisible = false;

  if (GetLocalBounds(bounds, bAlwaysVisible, msg).Succeeded())
  {
    plSpatialData::Category category = GetOwner()->IsDynamic() ? plDefaultSpatialDataCategories::RenderDynamic : plDefaultSpatialDataCategories::RenderStatic;

    if (bounds.IsValid())
    {
      msg.AddBounds(bounds, category);
    }

    if (bAlwaysVisible)
    {
      msg.SetAlwaysVisible(category);
    }
  }
}

void plRenderComponent::InvalidateCachedRenderData()
{
  if (IsActiveAndInitialized())
  {
    plRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void plRenderComponent::TriggerLocalBoundsUpdate()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

// static
plUInt32 plRenderComponent::GetUniqueIdForRendering(const plComponent* pComponent, plUInt32 uiInnerIndex /*= 0*/, plUInt32 uiInnerIndexShift /*= 24*/)
{
  plUInt32 uniqueId = pComponent->GetUniqueID();
  if (uniqueId == plInvalidIndex)
  {
    uniqueId = pComponent->GetOwner()->GetHandle().GetInternalID().m_InstanceIndex;
  }
  else
  {
    uniqueId |= (uiInnerIndex << uiInnerIndexShift);
  }

  const plUInt32 dynamicBit = (1 << 31);
  const plUInt32 dynamicBitMask = ~dynamicBit;
  return (uniqueId & dynamicBitMask) | (pComponent->GetOwner()->IsDynamic() ? dynamicBit : 0);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderComponent);
