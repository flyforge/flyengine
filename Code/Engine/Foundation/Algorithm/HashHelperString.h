
#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/StringUtils.h>

/// \brief Hash helper to be used as a template argument to plHashTable / plHashSet for case insensitive string keys.
struct PL_FOUNDATION_DLL plHashHelperString_NoCase
{
  inline static plUInt32 Hash(plStringView sValue); // [tested]

  PL_ALWAYS_INLINE static bool Equal(plStringView lhs, plStringView rhs); // [tested]
};

#include <Foundation/Algorithm/Implementation/HashHelperString_inl.h>
