#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioProxyComponent.h>
#include <AudioSystemPlugin/Components/AudioSphereEnvironmentComponent.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr plTypeVersion kVersion_AudioSphereEnvironmentComponent = 1;

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAudioSphereEnvironmentComponent, kVersion_AudioSphereEnvironmentComponent, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("Radius"),
    new plSphereVisualizerAttribute("Radius", plColor::White, "Color"),
    new plSphereVisualizerAttribute("MaxDistance", plColor::White, "Color"),
  }
  PLASMA_END_ATTRIBUTES;

  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, plVariant()), new plSuffixAttribute(" m")),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

void plAudioSphereEnvironmentComponent::Initialize()
{
  SUPER::Initialize();

  m_Sphere.m_vCenter = GetOwner()->GetGlobalPosition();
}

void plAudioSphereEnvironmentComponent::Deinitialize()
{
  SUPER::Deinitialize();
}

void plAudioSphereEnvironmentComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioSphereEnvironmentComponent);

  s << m_sEnvironmentName;
  s << m_Sphere.m_fRadius;
  s << m_fMaxDistance;
}

void plAudioSphereEnvironmentComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioSphereEnvironmentComponent);

  s >> m_sEnvironmentName;
  s >> m_Sphere.m_fRadius;
  s >> m_fMaxDistance;
}

plAudioSphereEnvironmentComponent::plAudioSphereEnvironmentComponent()
  : plAudioSystemEnvironmentComponent()
{
}

float plAudioSphereEnvironmentComponent::GetRadius() const
{
  return m_Sphere.m_fRadius;
}

void plAudioSphereEnvironmentComponent::SetRadius(float fRadius)
{
  m_Sphere.m_fRadius = fRadius;
}

float plAudioSphereEnvironmentComponent::GetEnvironmentAmount(plAudioProxyComponent* pProxyComponent) const
{
  if (m_bOverrideValue)
    return m_fOverrideValue;

  const plVec3& proxyPosition = pProxyComponent->GetOwner()->GetGlobalPosition();
  const float fDistanceToOrigin = (proxyPosition - m_Sphere.m_vCenter).GetLength();

  if (fDistanceToOrigin <= m_Sphere.m_fRadius)
    return 1.0f;

  if (fDistanceToOrigin >= m_fMaxDistance)
    return 0.0f;

  return plMath::Lerp(1.0f, 0.0f, (fDistanceToOrigin - m_fMaxDistance) / (m_Sphere.m_fRadius - m_fMaxDistance));
}

void plAudioSphereEnvironmentComponent::Update()
{
  m_Sphere.m_vCenter = GetOwner()->GetGlobalPosition();
}

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioSphereEnvironmentComponent);
