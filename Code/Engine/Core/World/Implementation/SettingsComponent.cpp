#include <Core/CorePCH.h>

#include <Core/World/SettingsComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSettingsComponent, 1, plRTTINoAllocator)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Settings"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSettingsComponent::plSettingsComponent()
{
  SetModified();
}

plSettingsComponent::~plSettingsComponent() = default;


PL_STATICLINK_FILE(Core, Core_World_Implementation_SettingsComponent);
