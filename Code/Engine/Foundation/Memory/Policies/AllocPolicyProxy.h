#pragma once

#include <Foundation/Basics.h>

/// \brief This Allocation policy redirects all operations to its parent.
///
/// \note Note that the stats are taken on the proxy as well as on the parent.
///
/// \see plAllocatorWithPolicy
class plAllocPolicyProxy
{
public:
  PL_FORCE_INLINE plAllocPolicyProxy(plAllocator* pParent)
    : m_pParent(pParent)
  {
    PL_ASSERT_ALWAYS(m_pParent != nullptr, "Parent allocator must not be nullptr");
  }

  PL_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign) { return m_pParent->Allocate(uiSize, uiAlign); }

  PL_FORCE_INLINE void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
  {
    return m_pParent->Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);
  }

  PL_FORCE_INLINE void Deallocate(void* pPtr) { m_pParent->Deallocate(pPtr); }

  PL_FORCE_INLINE size_t AllocatedSize(const void* pPtr) { return m_pParent->AllocatedSize(pPtr); }

  PL_ALWAYS_INLINE plAllocator* GetParent() const { return m_pParent; }

private:
  plAllocator* m_pParent;
};
