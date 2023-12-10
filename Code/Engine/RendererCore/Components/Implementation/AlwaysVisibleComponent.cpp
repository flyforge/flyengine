#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/AlwaysVisibleComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAlwaysVisibleComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plAlwaysVisibleComponent::plAlwaysVisibleComponent() = default;
plAlwaysVisibleComponent::~plAlwaysVisibleComponent() = default;

plResult plAlwaysVisibleComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_AlwaysVisibleComponent);
