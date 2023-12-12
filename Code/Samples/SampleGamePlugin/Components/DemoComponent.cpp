#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <SampleGamePlugin/Components/DemoComponent.h>

// BEGIN-DOCS-CODE-SNIPPET: customcomp-reflection
// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: component-reflection
PLASMA_BEGIN_COMPONENT_TYPE(DemoComponent, 3 /* version */, plComponentMode::Dynamic)
// END-DOCS-CODE-SNIPPET
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Amplitude", m_fAmplitude)->AddAttributes(new plDefaultValueAttribute(1), new plClampValueAttribute(0, 10)),
    PLASMA_MEMBER_PROPERTY("Speed", m_Speed)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(90))),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("SampleGamePlugin"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: customcomp-basics
DemoComponent::DemoComponent() = default;
DemoComponent::~DemoComponent() = default;

void DemoComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // this component doesn't need to anything for initialization
}

void DemoComponent::Update()
{
  const plTime curTime = GetWorld()->GetClock().GetAccumulatedTime();
  const plAngle curAngle = curTime.AsFloatInSeconds() * m_Speed;
  const float curHeight = plMath::Sin(curAngle) * m_fAmplitude;

  GetOwner()->SetLocalPosition(plVec3(0, 0, curHeight));
}

// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-serialize
void DemoComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fAmplitude;
  s << m_Speed;
}
// END-DOCS-CODE-SNIPPET

// BEGIN-DOCS-CODE-SNIPPET: component-deserialize
void DemoComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fAmplitude;

  if (uiVersion <= 2)
  {
    // up to version 2 the angle was stored as a float in degree
    // convert this to plAngle
    float fDegree;
    s >> fDegree;
    m_Speed = plAngle::Degree(fDegree);
  }
  else
  {
    s >> m_Speed;
  }
}
// END-DOCS-CODE-SNIPPET
