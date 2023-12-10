#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_Wait.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plScriptCoroutine_Wait, plScriptCoroutine, 1, plRTTIDefaultAllocator<plScriptCoroutine_Wait>)
{
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Start, In, "Timeout"),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Coroutine::Wait {Timeout}"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
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
    return Result::Running(m_TimeRemaing);
  }

  return Result::Completed();
}
