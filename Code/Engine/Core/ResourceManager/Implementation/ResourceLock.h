#pragma once

#include <Core/ResourceManager/Implementation/Declarations.h>

/// \brief Helper class to acquire and release a resource safely.
///
/// The constructor calls plResourceManager::BeginAcquireResource, the destructor makes sure to call plResourceManager::EndAcquireResource.
/// The instance of this class can be used like a pointer to the resource.
///
/// Whether the acquisition succeeded or returned a loading fallback, missing fallback or even no result, at all,
/// can be retrieved through GetAcquireResult().
/// \note If a resource is missing, but no missing fallback is specified for the resource type, the code will fail with an assertion,
/// unless you used plResourceAcquireMode::BlockTillLoaded_NeverFail. Only then will the error be silently ignored and the acquire result
/// will be plResourceAcquireResult::None.
///
/// \sa plResourceManager::BeginAcquireResource()
/// \sa plResourceAcquireMode
/// \sa plResourceAcquireResult
template <class RESOURCE_TYPE>
class plResourceLock
{
public:
  PLASMA_ALWAYS_INLINE plResourceLock(const plTypedResourceHandle<RESOURCE_TYPE>& hResource, plResourceAcquireMode mode,
    const plTypedResourceHandle<RESOURCE_TYPE>& hFallbackResource = plTypedResourceHandle<RESOURCE_TYPE>())
  {
    m_pResource = plResourceManager::BeginAcquireResource(hResource, mode, hFallbackResource, &m_AcquireResult);
  }

  plResourceLock(const plResourceLock&) = delete;

  plResourceLock(plResourceLock&& other)
    : m_AcquireResult(other.m_AcquireResult)
    , m_pResource(other.m_pResource)
  {
    other.m_pResource = nullptr;
    other.m_AcquireResult = plResourceAcquireResult::None;
  }

  PLASMA_ALWAYS_INLINE ~plResourceLock()
  {
    if (m_pResource)
    {
      plResourceManager::EndAcquireResource(m_pResource);
    }
  }

  PLASMA_ALWAYS_INLINE RESOURCE_TYPE* operator->() { return m_pResource; }
  PLASMA_ALWAYS_INLINE const RESOURCE_TYPE* operator->() const { return m_pResource; }

  PLASMA_ALWAYS_INLINE bool IsValid() const { return m_pResource != nullptr; }
  PLASMA_ALWAYS_INLINE explicit operator bool() const { return m_pResource != nullptr; }

  PLASMA_ALWAYS_INLINE plResourceAcquireResult GetAcquireResult() const { return m_AcquireResult; }

  PLASMA_ALWAYS_INLINE const RESOURCE_TYPE* GetPointer() const { return m_pResource; }
  PLASMA_ALWAYS_INLINE RESOURCE_TYPE* GetPointerNonConst() const { return m_pResource; }

private:
  plResourceAcquireResult m_AcquireResult;
  RESOURCE_TYPE* m_pResource;
};
