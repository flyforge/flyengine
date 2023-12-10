#pragma once

#ifdef BUILDSYSTEM_BUILDING_CORE_LIB
#  define PLASMA_CORE_INTERNAL_HEADER_ALLOWED 1
#else
#  define PLASMA_CORE_INTERNAL_HEADER_ALLOWED 0
#endif

#define PLASMA_CORE_INTERNAL_HEADER static_assert(PLASMA_CORE_INTERNAL_HEADER_ALLOWED, "This is an internal pl header. Please do not #include it directly.");
