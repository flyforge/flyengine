#pragma once

/// \file

/// Global settings for how to to compile PL.
/// Modify these settings as you needed in your project.


#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
#  undef PL_COMPILE_ENGINE_AS_DLL
#  define PL_COMPILE_ENGINE_AS_DLL PL_ON
#else
#  undef PL_COMPILE_ENGINE_AS_DLL
#  define PL_COMPILE_ENGINE_AS_DLL PL_OFF
#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Shipping)

// Development checks like assert.
#  undef PL_COMPILE_FOR_DEVELOPMENT
#  define PL_COMPILE_FOR_DEVELOPMENT PL_OFF

// Performance profiling features
#  undef PL_USE_PROFILING
#  define PL_USE_PROFILING PL_OFF

// Tracking of memory allocations.
#  undef PL_ALLOC_TRACKING_DEFAULT
#  define PL_ALLOC_TRACKING_DEFAULT plAllocatorTrackingMode::Nothing

#else

// Development checks like assert.
#  undef PL_COMPILE_FOR_DEVELOPMENT
#  define PL_COMPILE_FOR_DEVELOPMENT PL_ON

// Performance profiling features
#  undef PL_USE_PROFILING
#  define PL_USE_PROFILING PL_ON

// Tracking of memory allocations.
#  undef PL_ALLOC_TRACKING_DEFAULT
#  define PL_ALLOC_TRACKING_DEFAULT plAllocatorTrackingMode::AllocationStatsAndStacktraces

#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Debug)
#  undef PL_MATH_CHECK_FOR_NAN
#  define PL_MATH_CHECK_FOR_NAN PL_ON
#endif


/// Whether game objects compute and store their velocity since the last frame (increases object size)
#define PL_GAMEOBJECT_VELOCITY PL_ON

// Migration code path. Added in March 2023, should be removed after a 'save' time.
#undef PL_MIGRATE_RUNTIMECONFIGS
#define PL_MIGRATE_RUNTIMECONFIGS PL_ON
