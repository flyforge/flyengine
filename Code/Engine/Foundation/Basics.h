#pragma once

#include <Foundation/Basics/PreprocessorUtils.h>

#include <Foundation/Basics/AllDefinesOff.h>

#include <Foundation/Basics/Platform/DetectArchitecture.h>
#include <Foundation/Basics/Platform/DetectPlatform.h>

#include <Foundation/UserConfig.h>

// Configure the DLL Import/Export Define
#if PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#    define PL_FOUNDATION_DLL PL_DECL_EXPORT
#    define PL_FOUNDATION_DLL_FRIEND PL_DECL_EXPORT_FRIEND
#  else
#    define PL_FOUNDATION_DLL PL_DECL_IMPORT
#    define PL_FOUNDATION_DLL_FRIEND PL_DECL_IMPORT_FRIEND
#  endif
#else
#  define PL_FOUNDATION_DLL
#  define PL_FOUNDATION_DLL_FRIEND
#endif

#include <Foundation/FoundationInternal.h>

// include the different headers for the supported platforms
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/Platform_win.h>
#elif PL_ENABLED(PL_PLATFORM_OSX)
#  include <Foundation/Basics/Platform/OSX/Platform_OSX.h>
#elif PL_ENABLED(PL_PLATFORM_LINUX) || PL_ENABLED(PL_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Linux/Platform_Linux.h>
#else
#  error "Undefined platform!"
#endif

// Here all the different features that each platform supports are declared.
#include <Foundation/Basics/Platform/PlatformFeatures.h>

// Include this last, it will ensure the previous includes have setup everything correctly
#include <Foundation/Basics/Platform/CheckDefinitions.h>

// Include common definitions and macros (e.g. PL_CHECK_AT_COMPILETIME)
#include <Foundation/Basics/Platform/Common.h>

// Include magic preprocessor macros
#include <Foundation/Basics/Platform/BlackMagic.h>

// Now declare all fundamental types
#include <Foundation/Types/Types.h>

#ifdef BUILDSYSTEM_BUILDING_FOUNDATION_LIB
#  if BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && PL_DISABLED(PL_COMPILE_ENGINE_AS_DLL)
#    error "The Buildsystem is configured to build the Engine as a shared library, but PL_COMPILE_ENGINE_AS_DLL is not defined in UserConfig.h"
#  endif
#  if !BUILDSYSTEM_COMPILE_ENGINE_AS_DLL && PL_ENABLED(PL_COMPILE_ENGINE_AS_DLL)
#    error "The Buildsystem is configured to build the Engine as a static library, but PL_COMPILE_ENGINE_AS_DLL is defined in UserConfig.h"
#  endif
#endif

// Finally include the rest of basics
#include <Foundation/Basics/Assert.h>

#include <Foundation/Types/TypeTraits.h>

#include <Foundation/Memory/Allocator.h>

#include <Foundation/Configuration/StaticSubSystem.h>
#include <Foundation/Strings/FormatString.h>

class PL_FOUNDATION_DLL plFoundation
{
public:
  static plAllocator* s_pDefaultAllocator;
  static plAllocator* s_pAlignedAllocator;

  /// \brief The default allocator can be used for any kind of allocation if no alignment is required
  PL_ALWAYS_INLINE static plAllocator* GetDefaultAllocator()
  {
    if (s_bIsInitialized)
      return s_pDefaultAllocator;
    else // the default allocator is not yet set so we return the static allocator instead.
      return GetStaticsAllocator();
  }

  /// \brief The aligned allocator should be used for all allocations which need alignment
  PL_ALWAYS_INLINE static plAllocator* GetAlignedAllocator()
  {
    PL_ASSERT_RELEASE(s_pAlignedAllocator != nullptr, "plFoundation must have been initialized before this function can be called. This "
                                                      "error can occur when you have a global variable or a static member variable that "
                                                      "(indirectly) requires an allocator. Check out the documentation for 'plStatic' for "
                                                      "more information about this issue.");
    return s_pAlignedAllocator;
  }

  /// \brief Returns the allocator that is used by global data and static members before the default allocator is created.
  static plAllocator* GetStaticsAllocator();

private:
  friend class plStartup;
  friend struct plStaticsAllocatorWrapper;

  static void Initialize();
  static bool s_bIsInitialized;
};
