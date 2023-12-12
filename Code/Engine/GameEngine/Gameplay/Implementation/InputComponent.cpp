#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Gameplay/InputComponent.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plInputMessageGranularity, 1)
  PLASMA_ENUM_CONSTANT(plInputMessageGranularity::PressOnly),
  PLASMA_ENUM_CONSTANT(plInputMessageGranularity::PressAndRelease),
  PLASMA_ENUM_CONSTANT(plInputMessageGranularity::PressReleaseAndDown),
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgInputActionTriggered);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgInputActionTriggered, 1, plRTTIDefaultAllocator<plMsgInputActionTriggered>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("InputAction", GetInputAction, SetInputAction),
    PLASMA_MEMBER_PROPERTY("KeyPressValue", m_fKeyPressValue),
    PLASMA_ENUM_MEMBER_PROPERTY("TriggerState", plTriggerState, m_TriggerState),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_COMPONENT_TYPE(plInputComponent, 3, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("InputSet", m_sInputSet)->AddAttributes(new plDynamicStringEnumAttribute("InputSet")),
    PLASMA_ENUM_MEMBER_PROPERTY("Granularity", plInputMessageGranularity, m_Granularity),
    PLASMA_MEMBER_PROPERTY("ForwardToBlackboard", m_bForwardToBlackboard),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input"),
    new plColorAttribute(plColorScheme::Input),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGESENDERS
  {
    PLASMA_MESSAGE_SENDER(m_InputEventSender)
  }
  PLASMA_END_MESSAGESENDERS;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetCurrentInputState, In, "InputAction", In, "OnlyKeyPressed"),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plInputComponent::plInputComponent() = default;
plInputComponent::~plInputComponent() = default;

static inline plTriggerState::Enum ToTriggerState(plKeyState::Enum s)
{
  switch (s)
  {
    case plKeyState::Pressed:
      return plTriggerState::Activated;

    case plKeyState::Released:
      return plTriggerState::Deactivated;

    default:
      return plTriggerState::Continuing;
  }
}

void plInputComponent::Update()
{
  if (m_sInputSet.IsEmpty())
    return;

  plHybridArray<plString, 24> AllActions;
  plInputManager::GetAllInputActions(m_sInputSet, AllActions);

  plMsgInputActionTriggered msg;

  plBlackboard* pBlackboard = m_bForwardToBlackboard ? plBlackboardComponent::FindBlackboard(GetOwner()) : nullptr;

  for (const plString& actionName : AllActions)
  {
    float fValue = 0.0f;
    const plKeyState::Enum state = plInputManager::GetInputActionState(m_sInputSet, actionName, &fValue);

    if (pBlackboard)
    {
      // we don't mind if the entry doesn't exist, that just means nobody is interested in reading the value
      pBlackboard->SetEntryValue(plTempHashedString(actionName), fValue).IgnoreResult();
    }

    if (state == plKeyState::Up)
      continue;
    if (state == plKeyState::Down && m_Granularity < plInputMessageGranularity::PressReleaseAndDown)
      continue;
    if (state == plKeyState::Released && m_Granularity == plInputMessageGranularity::PressOnly)
      continue;

    msg.m_TriggerState = ToTriggerState(state);
    msg.m_sInputAction.Assign(actionName);
    msg.m_fKeyPressValue = fValue;

    m_InputEventSender.SendEventMessage(msg, this, GetOwner());
  }
}

float plInputComponent::GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed /*= false*/) const
{
  if (m_sInputSet.IsEmpty())
    return 0;

  float fValue = 0.0f;
  const plKeyState::Enum state = plInputManager::GetInputActionState(m_sInputSet, szInputAction, &fValue);

  if (bOnlyKeyPressed && state != plKeyState::Pressed)
    return 0;

  if (state != plKeyState::Up)
  {
    return fValue;
  }

  return fValue;
}

void plInputComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sInputSet;
  s << m_Granularity;

  // version 3
  s << m_bForwardToBlackboard;
}

void plInputComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();


  s >> m_sInputSet;
  s >> m_Granularity;

  if (uiVersion >= 3)
  {
    s >> m_bForwardToBlackboard;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plInputComponentPatch_1_2 : public plGraphPatch
{
public:
  plInputComponentPatch_1_2()
    : plGraphPatch("plInputComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Input Set", "InputSet");
  }
};

plInputComponentPatch_1_2 g_plInputComponentPatch_1_2;



PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_InputComponent);
