#pragma once

#include <Core/Scripting/DuktapeFunction.h>
#include <Foundation/Memory/CommonAllocators.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

struct duk_hthread;
typedef duk_hthread duk_context;
typedef int (*duk_c_function)(duk_context* ctx);


class PLASMA_CORE_DLL plDuktapeContext : public plDuktapeHelper
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plDuktapeContext);

public:
  plDuktapeContext(plStringView sWrapperName);
  ~plDuktapeContext();

  /// \name Basics
  ///@{

  /// \brief Enables support for loading modules via the 'require' function
  void EnableModuleSupport(duk_c_function moduleSearchFunction);

  ///@}

private:
  void InitializeContext();
  void DestroyContext();

  static void FatalErrorHandler(void* pUserData, const char* szMsg);
  static void* DukAlloc(void* pUserData, size_t size);
  static void* DukRealloc(void* pUserData, void* pPointer, size_t size);
  static void DukFree(void* pUserData, void* pPointer);

protected:
  bool m_bInitializedModuleSupport = false;

private:
#  if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  plAllocator<plMemoryPolicies::plHeapAllocation, plMemoryTrackingFlags::RegisterAllocator | plMemoryTrackingFlags::EnableAllocationTracking>
    m_Allocator;
#  else
  plAllocator<plMemoryPolicies::plHeapAllocation, plMemoryTrackingFlags::None> m_Allocator;
#  endif
};

#endif // BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT
