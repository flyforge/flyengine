#include <Foundation/Memory/Policies/AllocPolicyGuarding.h>

plAllocPolicyGuarding::plAllocPolicyGuarding(plAllocator* pParent)
{
  PL_ASSERT_NOT_IMPLEMENTED;
  PL_IGNORE_UNUSED(m_uiPageSize);
  PL_IGNORE_UNUSED(m_Mutex);
  PL_IGNORE_UNUSED(m_AllocationsToFreeLater);
}

void* plAllocPolicyGuarding::Allocate(size_t uiSize, size_t uiAlign)
{
  PL_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
}

void plAllocPolicyGuarding::Deallocate(void* ptr)
{
  PL_ASSERT_NOT_IMPLEMENTED;
}
