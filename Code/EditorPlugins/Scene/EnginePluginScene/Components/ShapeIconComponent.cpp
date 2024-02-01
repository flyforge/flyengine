#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/Components/ShapeIconComponent.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plShapeIconComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Editing"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plShapeIconComponent::plShapeIconComponent() = default;
plShapeIconComponent::~plShapeIconComponent() = default;
