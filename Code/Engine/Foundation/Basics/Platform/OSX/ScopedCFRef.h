#pragma once

#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <CoreFoundation/CoreFoundation.h>

/// \brief Helper class to release references of core foundation objects correctly.
template <typename T>
class plScopedCFRef
{
public:
  plScopedCFRef(T Ref)
    : m_Ref(Ref)
  {
  }

  ~plScopedCFRef()
  {
    CFRelease(m_Ref);
  }

  operator T() const
  {
    return m_Ref;
  }

private:
  T m_Ref;
};
