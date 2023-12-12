#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltSettingsComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("ObjectGravity", GetObjectGravity, SetObjectGravity)->AddAttributes(new plDefaultValueAttribute(plVec3(0, 0, -9.81f))),
    PLASMA_ACCESSOR_PROPERTY("CharacterGravity", GetCharacterGravity, SetCharacterGravity)->AddAttributes(new plDefaultValueAttribute(plVec3(0, 0, -12.0f))),
    PLASMA_ENUM_ACCESSOR_PROPERTY("SteppingMode", plJoltSteppingMode, GetSteppingMode, SetSteppingMode),
    PLASMA_ACCESSOR_PROPERTY("FixedFrameRate", GetFixedFrameRate, SetFixedFrameRate)->AddAttributes(new plDefaultValueAttribute(60.0f), new plClampValueAttribute(1.0f, 1000.0f)),
    PLASMA_ACCESSOR_PROPERTY("MaxSubSteps", GetMaxSubSteps, SetMaxSubSteps)->AddAttributes(new plDefaultValueAttribute(4), new plClampValueAttribute(1, 100)),
    PLASMA_ACCESSOR_PROPERTY("MaxBodies", GetMaxBodies, SetMaxBodies)->AddAttributes(new plDefaultValueAttribute(10000), new plClampValueAttribute(500, 1000000)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Misc"),
    new plColorAttribute(plColorScheme::Physics),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltSettingsComponent::plJoltSettingsComponent() = default;
plJoltSettingsComponent::~plJoltSettingsComponent() = default;

void plJoltSettingsComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_Settings.m_vObjectGravity;
  s << m_Settings.m_vCharacterGravity;
  s << m_Settings.m_SteppingMode;
  s << m_Settings.m_fFixedFrameRate;
  s << m_Settings.m_uiMaxSubSteps;
  s << m_Settings.m_uiMaxBodies;
}


void plJoltSettingsComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_Settings.m_vObjectGravity;
  s >> m_Settings.m_vCharacterGravity;
  s >> m_Settings.m_SteppingMode;
  s >> m_Settings.m_fFixedFrameRate;
  s >> m_Settings.m_uiMaxSubSteps;
  s >> m_Settings.m_uiMaxBodies;
}

void plJoltSettingsComponent::SetObjectGravity(const plVec3& v)
{
  m_Settings.m_vObjectGravity = v;
  SetModified(PLASMA_BIT(0));
}

void plJoltSettingsComponent::SetCharacterGravity(const plVec3& v)
{
  m_Settings.m_vCharacterGravity = v;
  SetModified(PLASMA_BIT(1));
}

void plJoltSettingsComponent::SetSteppingMode(plJoltSteppingMode::Enum mode)
{
  m_Settings.m_SteppingMode = mode;
  SetModified(PLASMA_BIT(3));
}

void plJoltSettingsComponent::SetFixedFrameRate(float fFixedFrameRate)
{
  m_Settings.m_fFixedFrameRate = fFixedFrameRate;
  SetModified(PLASMA_BIT(4));
}

void plJoltSettingsComponent::SetMaxSubSteps(plUInt32 uiMaxSubSteps)
{
  m_Settings.m_uiMaxSubSteps = uiMaxSubSteps;
  SetModified(PLASMA_BIT(5));
}

void plJoltSettingsComponent::SetMaxBodies(plUInt32 uiMaxBodies)
{
  m_Settings.m_uiMaxBodies = uiMaxBodies;
  SetModified(PLASMA_BIT(6));
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltSettingsComponent);

