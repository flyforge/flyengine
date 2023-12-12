#include <CppProjectPlugin/CppProjectPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <CppProjectPlugin/Components/SampleBounceComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(SampleBounceComponent, 1 /* version */, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(0, 10)),
    PLASMA_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(90))),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("CppProject"), // Component menu group
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

SampleBounceComponent::SampleBounceComponent() = default;
SampleBounceComponent::~SampleBounceComponent() = default;

void SampleBounceComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void SampleBounceComponent::Update()
{
  const plTime curTime = GetWorld()->GetClock().GetAccumulatedTime();
  const plAngle curAngle = curTime.AsFloatInSeconds() * m_Speed;
  const float curHeight = plMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(plVec3(0, 0, curHeight));
}

void SampleBounceComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fAmplitude;
  s << m_Speed;
}

void SampleBounceComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fAmplitude;
  s >> m_Speed;
}
