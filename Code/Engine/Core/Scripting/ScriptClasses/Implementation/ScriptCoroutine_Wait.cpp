#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_Wait.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plScriptCoroutine_Wait, plScriptCoroutine, 1, plRTTIDefaultAllocator<plScriptCoroutine_Wait>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(Start, In, "Timeout"),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Coroutine::Wait {Timeout}"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plScriptCoroutine_Wait::Start(plTime timeout)
{
  m_TimeRemaing = timeout;
}

plScriptCoroutine::Result plScriptCoroutine_Wait::Update(plTime deltaTimeSinceLastUpdate)
{
  m_TimeRemaing -= deltaTimeSinceLastUpdate;
  if (m_TimeRemaing.IsPositive())
  {
    // Don't wait for the full remaining time to prevent oversleeping due to scheduling precision.
    return Result::Running(m_TimeRemaing * 0.8);
  }

  return Result::Completed();
}


PL_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_Wait);

