#pragma once

#include <Foundation/Basics.h>

namespace plMemoryPolicies
{
  /// \brief This Allocation policy redirects all operations to its parent.
  ///
  /// \note Note that the stats are taken on the proxy as well as on the parent.
  ///
  /// \see plAllocator
  class plProxyAllocation
  {
  public:
    PLASMA_FORCE_INLINE plProxyAllocation(plAllocatorBase* pParent)
      : m_pParent(pParent)
    {
      PLASMA_ASSERT_ALWAYS(m_pParent != nullptr, "Parent allocator must not be nullptr");
    }

    PLASMA_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign) { return m_pParent->Allocate(uiSize, uiAlign); }

    PLASMA_FORCE_INLINE void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
    {
      return m_pParent->Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);
    }

    PLASMA_FORCE_INLINE void Deallocate(void* pPtr) { m_pParent->Deallocate(pPtr); }

    PLASMA_FORCE_INLINE size_t AllocatedSize(const void* pPtr) { return m_pParent->AllocatedSize(pPtr); }

    PLASMA_ALWAYS_INLINE plAllocatorBase* GetParent() const { return m_pParent; }

  private:
    plAllocatorBase* m_pParent;
  };
} // namespace plMemoryPolicies
