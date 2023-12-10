#include <Core/CorePCH.h>

#include <Core/Scripting/DuktapeContext.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>

plDuktapeContext::plDuktapeContext(plStringView sWrapperName)
  : plDuktapeHelper(nullptr)
  , m_Allocator(sWrapperName, plFoundation::GetDefaultAllocator())

{
  InitializeContext();
}

plDuktapeContext::~plDuktapeContext()
{
  DestroyContext();
}

void plDuktapeContext::EnableModuleSupport(duk_c_function moduleSearchFunction)
{
  if (!m_bInitializedModuleSupport)
  {
    // we need an 'exports' object for the transpiled TypeScript files to load
    // (they access this object to define the '__esModule' property on it)
    ExecuteString("var exports = {}").IgnoreResult();

    m_bInitializedModuleSupport = true;
    duk_module_duktape_init(m_pContext);
  }

  if (moduleSearchFunction)
  {
    duk_get_global_string(m_pContext, "Duktape");
    duk_push_c_function(m_pContext, moduleSearchFunction, 4);
    duk_put_prop_string(m_pContext, -2, "modSearch");
    duk_pop(m_pContext);
  }
}

void plDuktapeContext::InitializeContext()
{
  PLASMA_ASSERT_ALWAYS(m_pContext == nullptr, "Duktape context should be null");

  m_pContext = duk_create_heap(DukAlloc, DukRealloc, DukFree, this, FatalErrorHandler);

  PLASMA_ASSERT_ALWAYS(m_pContext != nullptr, "Duktape context could not be created");
}

void plDuktapeContext::DestroyContext()
{
  duk_destroy_heap(m_pContext);
  m_pContext = nullptr;

  const auto stats = m_Allocator.GetStats();
  PLASMA_ASSERT_DEBUG(stats.m_uiAllocationSize == 0, "Duktape did not free all data");
  PLASMA_ASSERT_DEBUG(stats.m_uiNumAllocations == stats.m_uiNumDeallocations, "Duktape did not free all data");
}

void plDuktapeContext::FatalErrorHandler(void* pUserData, const char* szMsg)
{
  // unfortunately it is not possible to do a stack trace here
  plLog::Error("DukTape: {}", szMsg);
  PLASMA_ASSERT_ALWAYS(false, "Duktape fatal error {}", szMsg);
}

void* plDuktapeContext::DukAlloc(void* pUserData, size_t size)
{
  PLASMA_ASSERT_DEBUG(size > 0, "Invalid allocation");

  plDuktapeContext* pDukWrapper = reinterpret_cast<plDuktapeContext*>(pUserData);

  return pDukWrapper->m_Allocator.Allocate(size, 8);
}

void* plDuktapeContext::DukRealloc(void* pUserData, void* pPointer, size_t size)
{
  plDuktapeContext* pDukWrapper = reinterpret_cast<plDuktapeContext*>(pUserData);

  if (size == 0)
  {
    if (pPointer != nullptr)
    {
      pDukWrapper->m_Allocator.Deallocate(pPointer);
    }

    return nullptr;
  }

  if (pPointer == nullptr)
  {
    return pDukWrapper->m_Allocator.Allocate(size, 8);
  }
  else
  {
    return pDukWrapper->m_Allocator.Reallocate(pPointer, pDukWrapper->m_Allocator.AllocatedSize(pPointer), size, 8);
  }
}

void plDuktapeContext::DukFree(void* pUserData, void* pPointer)
{
  if (pPointer == nullptr)
    return;

  plDuktapeContext* pDukWrapper = reinterpret_cast<plDuktapeContext*>(pUserData);

  pDukWrapper->m_Allocator.Deallocate(pPointer);
}

#endif


PLASMA_STATICLINK_FILE(Core, Core_Scripting_Duktape_DuktapeContext);
