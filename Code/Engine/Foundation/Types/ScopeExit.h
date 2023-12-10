
#pragma once

#include <Foundation/Basics.h>

/// \file

/// \brief Macro to execute a piece of code when the current scope closes.
#define PLASMA_SCOPE_EXIT(code) auto PLASMA_CONCAT(scopeExit_, PLASMA_SOURCE_LINE) = plMakeScopeExit([&]() { code; })

/// \internal Helper class to implement PLASMA_SCOPE_EXIT
template <typename T>
struct plScopeExit
{
  PLASMA_ALWAYS_INLINE plScopeExit(T&& func)
    : m_func(std::forward<T>(func))
  {
  }

  PLASMA_ALWAYS_INLINE ~plScopeExit() { m_func(); }

  T m_func;
};

/// \internal Helper function to implement PLASMA_SCOPE_EXIT
template <typename T>
PLASMA_ALWAYS_INLINE plScopeExit<T> plMakeScopeExit(T&& func)
{
  return plScopeExit<T>(std::forward<T>(func));
}
