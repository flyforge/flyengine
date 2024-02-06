#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioBoxEnvironmentComponent.h>
#include <AudioSystemPlugin/Components/AudioProxyComponent.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr plTypeVersion kVersion_AudioBoxEnvironmentComponent = 1;

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAudioBoxEnvironmentComponent, kVersion_AudioBoxEnvironmentComponent, plComponentMode::Static)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plBoxManipulatorAttribute("HalfExtends", 1.0f, true),
    new plBoxVisualizerAttribute("HalfExtends", 1.0f, plColor::White, "Color"),
    new plSphereVisualizerAttribute("MaxDistance", plColor::White, "Color"),
  }
  PL_END_ATTRIBUTES;

  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("HalfExtends", GetHalfExtends, SetHalfExtends)->AddAttributes(new plDefaultValueAttribute(plVec3(0.5f)), new plClampValueAttribute(0.1f, plVariant()), new plSuffixAttribute(" m")),
  }
  PL_END_PROPERTIES;
}
PL_END_COMPONENT_TYPE;
// clang-format on


void plAudioBoxEnvironmentComponent::Initialize()
{
  SUPER::Initialize();

  m_Box = plBoundingBox::MakeFromCenterAndHalfExtents(GetOwner()->GetGlobalPosition(), m_vHalfExtends);
}

void plAudioBoxEnvironmentComponent::Deinitialize()
{
  SUPER::Deinitialize();
}

void plAudioBoxEnvironmentComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioBoxEnvironmentComponent);

  s << m_sEnvironmentName;
  s << m_vHalfExtends;
  s << m_fMaxDistance;
}

void plAudioBoxEnvironmentComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioBoxEnvironmentComponent);

  s >> m_sEnvironmentName;
  s >> m_vHalfExtends;
  s >> m_fMaxDistance;
}

float plAudioBoxEnvironmentComponent::GetEnvironmentAmount(plAudioProxyComponent* pProxyComponent) const
{
  if (m_bOverrideValue)
    return m_fOverrideValue;

  const plVec3& proxyPosition = pProxyComponent->GetOwner()->GetGlobalPosition();
  const plVec3& boxCenter = m_Box.GetCenter();

  if (m_Box.Contains(proxyPosition))
    return 1.0f;

  const plVec3& direction = (proxyPosition - boxCenter);
  const float fDistanceToOrigin = direction.GetLength();

  if (fDistanceToOrigin >= m_fMaxDistance)
    return 0.0f;

  plVec3 startPoint;
  m_Box.GetRayIntersection(boxCenter, direction, nullptr, &startPoint);
  const float fDistanceToBox = (startPoint - boxCenter).GetLength();

  return plMath::Lerp(1.0f, 0.0f, (fDistanceToOrigin - m_fMaxDistance) / (fDistanceToBox - m_fMaxDistance));
}

const plVec3& plAudioBoxEnvironmentComponent::GetHalfExtends() const
{
  return m_vHalfExtends;
}

void plAudioBoxEnvironmentComponent::SetHalfExtends(const plVec3& vHalfExtends)
{
  m_vHalfExtends = vHalfExtends;
}

void plAudioBoxEnvironmentComponent::Update()
{
  m_Box = plBoundingBox::MakeFromCenterAndHalfExtents(GetOwner()->GetGlobalPosition(), m_vHalfExtends);
}

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioBoxEnvironmentComponent);
