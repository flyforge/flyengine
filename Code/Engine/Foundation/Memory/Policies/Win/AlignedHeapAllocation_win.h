
PLASMA_FORCE_INLINE void* plAlignedHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  uiAlign = plMath::Max<size_t>(uiAlign, 16u);

  void* ptr = _aligned_malloc(uiSize, uiAlign);
  PLASMA_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

PLASMA_ALWAYS_INLINE void plAlignedHeapAllocation::Deallocate(void* pPtr)
{
  _aligned_free(pPtr);
}
