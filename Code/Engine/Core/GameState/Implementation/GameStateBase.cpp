#include <Core/CorePCH.h>

#include <Core/GameState/GameStateBase.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plGameStateBase, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plGameStateBase::plGameStateBase() = default;
plGameStateBase::~plGameStateBase() = default;

void plGameStateBase::ProcessInput() {}

void plGameStateBase::BeforeWorldUpdate() {}

void plGameStateBase::AfterWorldUpdate() {}

void plGameStateBase::RequestQuit()
{
  m_bStateWantsToQuit = true;
}

bool plGameStateBase::WasQuitRequested() const
{
  return m_bStateWantsToQuit;
}



PL_STATICLINK_FILE(Core, Core_GameState_Implementation_GameStateBase);
