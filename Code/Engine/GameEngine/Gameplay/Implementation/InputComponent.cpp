#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Gameplay/InputComponent.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plInputMessageGranularity, 1)
  PL_ENUM_CONSTANT(plInputMessageGranularity::PressOnly),
  PL_ENUM_CONSTANT(plInputMessageGranularity::PressAndRelease),
  PL_ENUM_CONSTANT(plInputMessageGranularity::PressReleaseAndDown),
PL_END_STATIC_REFLECTED_ENUM;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgInputActionTriggered);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgInputActionTriggered, 1, plRTTIDefaultAllocator<plMsgInputActionTriggered>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("InputAction", GetInputAction, SetInputAction),
    PL_MEMBER_PROPERTY("KeyPressValue", m_fKeyPressValue),
    PL_ENUM_MEMBER_PROPERTY("TriggerState", plTriggerState, m_TriggerState),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plInputComponent, 3, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("InputSet", m_sInputSet)->AddAttributes(new plDynamicStringEnumAttribute("InputSet")),
    PL_ENUM_MEMBER_PROPERTY("Granularity", plInputMessageGranularity, m_Granularity),
    PL_MEMBER_PROPERTY("ForwardToBlackboard", m_bForwardToBlackboard),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGESENDERS
  {
    PL_MESSAGE_SENDER(m_InputEventSender)
  }
  PL_END_MESSAGESENDERS;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(GetCurrentInputState, In, "InputAction", In, "OnlyKeyPressed"),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
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
      pBlackboard->SetEntryValue(actionName, fValue);
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

  return fValue;
}

void plInputComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sInputSet;
  s << m_Granularity;

  // version 3
  s << m_bForwardToBlackboard;
}

void plInputComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();


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

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Input Set", "InputSet");
  }
};

plInputComponentPatch_1_2 g_plInputComponentPatch_1_2;



PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_InputComponent);
