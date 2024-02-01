#pragma once

#ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#  define PL_FOUNDATION_INTERNAL_HEADER_ALLOWED 1
#else
#  define PL_FOUNDATION_INTERNAL_HEADER_ALLOWED 0
#endif

#define PL_FOUNDATION_INTERNAL_HEADER static_assert(PL_FOUNDATION_INTERNAL_HEADER_ALLOWED, "This is an internal pl header. Please do not #include it directly.");
