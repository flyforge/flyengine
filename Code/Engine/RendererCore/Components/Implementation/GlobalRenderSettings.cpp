#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Components/GlobalRenderSettings.h>


PLASMA_BEGIN_COMPONENT_TYPE(plGlobalRenderSettingsComponent, 1, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("TaaActive", GetTaaActive, SetTaaActive)->AddAttributes(new plGroupAttribute("Temporal Anti-Aliasing")),
    PLASMA_ACCESSOR_PROPERTY("TaaUpscale", GetTaaUpscaleActive, SetTaaUpscaleActive)->AddAttributes(new plGroupAttribute("Temporal Anti-Aliasing")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE

plGlobalRenderSettingsComponent::plGlobalRenderSettingsComponent() = default;
plGlobalRenderSettingsComponent::~plGlobalRenderSettingsComponent() = default;

void plGlobalRenderSettingsComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_bTaaActive;
}

void plGlobalRenderSettingsComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s >> m_bTaaActive;
}

void plGlobalRenderSettingsComponent::SetTaaActive(bool bActive)
{
  m_bTaaActive = bActive;
  plRenderWorld::SetTAAEnabled(bActive);
}

bool plGlobalRenderSettingsComponent::GetTaaActive() const
{
  return plRenderWorld::GetTAAEnabled();
}

void plGlobalRenderSettingsComponent::SetTaaUpscaleActive(bool bActive)
{
  m_bTaaUpscaleActive = bActive;
  plRenderWorld::SetTAAUpscaleEnabled(bActive);
}

bool plGlobalRenderSettingsComponent::GetTaaUpscaleActive() const
{
  return plRenderWorld::GetTAAUpscaleEnabled();
}


void plGlobalRenderSettingsComponent::OnActivated()
{
  SUPER::OnActivated();

  plRenderWorld::SetTAAEnabled(m_bTaaActive);
}

void plGlobalRenderSettingsComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  plRenderWorld::SetTAAEnabled(false);
}

void plGlobalRenderSettingsComponent::Update()
{
}