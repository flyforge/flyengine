#pragma once

PLASMA_ALWAYS_INLINE bool plPathUtils::IsPathSeparator(plUInt32 c)
{
  return (c == '/' || c == '\\');
}
