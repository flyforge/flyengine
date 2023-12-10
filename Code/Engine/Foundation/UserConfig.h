#pragma once

/// \file

/// Global settings for how to to compile PLASMA.
/// Modify these settings as you needed in your project.


#ifdef BUILDSYSTEM_COMPILE_ENGINE_AS_DLL
#  undef PLASMA_COMPILE_ENGINE_AS_DLL
#  define PLASMA_COMPILE_ENGINE_AS_DLL PLASMA_ON
#else
#  undef PLASMA_COMPILE_ENGINE_AS_DLL
#  define PLASMA_COMPILE_ENGINE_AS_DLL PLASMA_OFF
#endif

#if defined(BUILDSYSTEM_BUILDTYPE_Shipping)

// Development checks like assert.
#  undef PLASMA_COMPILE_FOR_DEVELOPMENT
#  define PLASMA_COMPILE_FOR_DEVELOPMENT PLASMA_OFF

// Performance profiling features
#  undef PLASMA_USE_PROFILING
#  define PLASMA_USE_PROFILING PLASMA_OFF

// Tracking of memory allocations.
#  undef PLASMA_USE_ALLOCATION_TRACKING
#  define PLASMA_USE_ALLOCATION_TRACKING PLASMA_OFF

// Stack traces for memory allocations.
#  undef PLASMA_USE_ALLOCATION_STACK_TRACING
#  define PLASMA_USE_ALLOCATION_STACK_TRACING PLASMA_OFF

#else

// Development checks like assert.
#  undef PLASMA_COMPILE_FOR_DEVELOPMENT
#  define PLASMA_COMPILE_FOR_DEVELOPMENT PLASMA_ON

// Performance profiling features
#  undef PLASMA_USE_PROFILING
#  define PLASMA_USE_PROFILING PLASMA_ON

// Tracking of memory allocations.
#  undef PLASMA_USE_ALLOCATION_TRACKING
#  define PLASMA_USE_ALLOCATION_TRACKING PLASMA_ON

// Stack traces for memory allocations.
#  undef PLASMA_USE_ALLOCATION_STACK_TRACING
#  define PLASMA_USE_ALLOCATION_STACK_TRACING PLASMA_ON

#endif

/// Whether game objects compute and store their velocity since the last frame (increases object size)
#define PLASMA_GAMEOBJECT_VELOCITY PLASMA_ON

// Migration code path. Added in March 2023, should be removed after a 'save' time.
#undef PLASMA_MIGRATE_RUNTIMECONFIGS
#define PLASMA_MIGRATE_RUNTIMECONFIGS PLASMA_ON
