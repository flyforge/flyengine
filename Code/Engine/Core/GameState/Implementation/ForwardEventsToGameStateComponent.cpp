#include <Core/GameApplication/GameApplicationBase.h>
#include <Core/GameState/ForwardEventsToGameStateComponent.h>
#include <Core/GameState/GameStateBase.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plForwardEventsToGameStateComponent, 1 /* version */, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay/Logic"),
    new plColorAttribute(plColorScheme::Logic),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plForwardEventsToGameStateComponent::plForwardEventsToGameStateComponent() = default;
plForwardEventsToGameStateComponent::~plForwardEventsToGameStateComponent() = default;

bool plForwardEventsToGameStateComponent::HandlesMessage(const plMessage& msg) const
{
  // check whether there is any active game state
  // if so, test whether it would handle this type of message
  if (plGameStateBase* pGameState = plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->CanHandleMessage(msg.GetId());
  }

  return false;
}

bool plForwardEventsToGameStateComponent::OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg)
{
  // if we have an active game state, forward the message to it
  if (plGameStateBase* pGameState = plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

bool plForwardEventsToGameStateComponent::OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg) const
{
  // if we have an active game state, forward the message to it
  if (const plGameStateBase* pGameState = plGameApplicationBase::GetGameApplicationBaseInstance()->GetActiveGameState())
  {
    return pGameState->GetDynamicRTTI()->DispatchMessage(pGameState, msg);
  }

  return false;
}

void plForwardEventsToGameStateComponent::Initialize()
{
  SUPER::Initialize();

  EnableUnhandledMessageHandler(true);
}
