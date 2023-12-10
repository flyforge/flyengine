
namespace plMemoryPolicies
{
  plGuardedAllocation::plGuardedAllocation(plAllocatorBase* pParent) { PLASMA_ASSERT_NOT_IMPLEMENTED; }

  void* plGuardedAllocation::Allocate(size_t uiSize, size_t uiAlign)
  {
    PLASMA_ASSERT_NOT_IMPLEMENTED;
    return nullptr;
  }

  void plGuardedAllocation::Deallocate(void* ptr) { PLASMA_ASSERT_NOT_IMPLEMENTED; }
} // namespace plMemoryPolicies
