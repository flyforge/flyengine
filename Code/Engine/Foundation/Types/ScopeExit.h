
#pragma once

#include <Foundation/Basics.h>

/// \file

/// \brief Macro to execute a piece of code when the current scope closes.
#define PL_SCOPE_EXIT(code) auto PL_CONCAT(scopeExit_, PL_SOURCE_LINE) = plMakeScopeExit([&]() { code; })

/// \internal Helper class to implement PL_SCOPE_EXIT
template <typename T>
struct plScopeExit
{
  PL_ALWAYS_INLINE plScopeExit(T&& func)
    : m_func(std::forward<T>(func))
  {
  }

  PL_ALWAYS_INLINE ~plScopeExit() { m_func(); }

  T m_func;
};

/// \internal Helper function to implement PL_SCOPE_EXIT
template <typename T>
PL_ALWAYS_INLINE plScopeExit<T> plMakeScopeExit(T&& func)
{
  return plScopeExit<T>(std::forward<T>(func));
}
