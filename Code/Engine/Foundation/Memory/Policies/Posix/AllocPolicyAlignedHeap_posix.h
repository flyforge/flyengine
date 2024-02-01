
PL_FORCE_INLINE void* plAllocPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  // alignment has to be at least sizeof(void*) otherwise posix_memalign will fail
  uiAlign = plMath::Max<size_t>(uiAlign, 16u);

  void* ptr = nullptr;

  int res = posix_memalign(&ptr, uiAlign, uiSize);
  PL_IGNORE_UNUSED(res);
  PL_ASSERT_DEV(res == 0, "posix_memalign failed with error: {0}", res);

  PL_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

PL_ALWAYS_INLINE void plAllocPolicyAlignedHeap::Deallocate(void* ptr)
{
  free(ptr);
}
