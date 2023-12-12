#include <Core/CorePCH.h>

#include <Core/World/SettingsComponent.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSettingsComponent, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Settings"),
    new plColorAttribute(plColorScheme::Utilities),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSettingsComponent::plSettingsComponent()
{
  SetModified();
}

plSettingsComponent::~plSettingsComponent() = default;


PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_SettingsComponent);
