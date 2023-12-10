
PLASMA_FORCE_INLINE void* plAlignedHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  // alignment has to be at least sizeof(void*) otherwise posix_memalign will fail
  uiAlign = plMath::Max<size_t>(uiAlign, 16u);

  void* ptr = nullptr;

  int res = posix_memalign(&ptr, uiAlign, uiSize);
  PLASMA_IGNORE_UNUSED(res);
  PLASMA_ASSERT_DEV(res == 0, "posix_memalign failed with error: {0}", res);

  PLASMA_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

PLASMA_ALWAYS_INLINE void plAlignedHeapAllocation::Deallocate(void* ptr)
{
  free(ptr);
}
