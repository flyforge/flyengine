
PL_FORCE_INLINE void* plAllocPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  uiAlign = plMath::Max<size_t>(uiAlign, 16u);

  void* ptr = _aligned_malloc(uiSize, uiAlign);
  PL_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

PL_ALWAYS_INLINE void plAllocPolicyAlignedHeap::Deallocate(void* pPtr)
{
  _aligned_free(pPtr);
}
