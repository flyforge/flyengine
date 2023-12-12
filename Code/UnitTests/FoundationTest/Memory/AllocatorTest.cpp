#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Memory/LargeBlockAllocator.h>
#include <Foundation/Memory/StackAllocator.h>

struct alignas(PLASMA_ALIGNMENT_MINIMUM) NonAlignedVector
{
  PLASMA_DECLARE_POD_TYPE();

  NonAlignedVector()
  {
    x = 5.0f;
    y = 6.0f;
    z = 8.0f;
  }

  float x;
  float y;
  float z;
};

struct alignas(16) AlignedVector
{
  PLASMA_DECLARE_POD_TYPE();

  AlignedVector()
  {
    x = 5.0f;
    y = 6.0f;
    z = 8.0f;
  }

  float x;
  float y;
  float z;
  float w;
};

template <typename T>
void TestAlignmentHelper(size_t uiExpectedAlignment)
{
  plAllocatorBase* pAllocator = plFoundation::GetAlignedAllocator();
  PLASMA_TEST_BOOL(pAllocator != nullptr);

  size_t uiAlignment = PLASMA_ALIGNMENT_OF(T);
  PLASMA_TEST_INT(uiAlignment, uiExpectedAlignment);

  T testOnStack = T();
  PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(&testOnStack, uiExpectedAlignment));

  T* pTestBuffer = PLASMA_NEW_RAW_BUFFER(pAllocator, T, 32);
  plArrayPtr<T> TestArray = PLASMA_NEW_ARRAY(pAllocator, T, 32);

  // default constructor should be called even if we declare as a pod type
  PLASMA_TEST_FLOAT(TestArray[0].x, 5.0f, 0.0f);
  PLASMA_TEST_FLOAT(TestArray[0].y, 6.0f, 0.0f);
  PLASMA_TEST_FLOAT(TestArray[0].z, 8.0f, 0.0f);

  PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(pTestBuffer, uiExpectedAlignment));
  PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(TestArray.GetPtr(), uiExpectedAlignment));

  size_t uiExpectedSize = sizeof(T) * 32;

#if PLASMA_ENABLED(PLASMA_USE_ALLOCATION_TRACKING)
  PLASMA_TEST_INT(pAllocator->AllocatedSize(pTestBuffer), uiExpectedSize);

  plAllocatorBase::Stats stats = pAllocator->GetStats();
  PLASMA_TEST_INT(stats.m_uiAllocationSize, uiExpectedSize * 2);
  PLASMA_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 2);
#endif

  PLASMA_DELETE_ARRAY(pAllocator, TestArray);
  PLASMA_DELETE_RAW_BUFFER(pAllocator, pTestBuffer);

#if PLASMA_ENABLED(PLASMA_USE_ALLOCATION_TRACKING)
  stats = pAllocator->GetStats();
  PLASMA_TEST_INT(stats.m_uiAllocationSize, 0);
  PLASMA_TEST_INT(stats.m_uiNumAllocations - stats.m_uiNumDeallocations, 0);
#endif
}

PLASMA_CREATE_SIMPLE_TEST_GROUP(Memory);

PLASMA_CREATE_SIMPLE_TEST(Memory, Allocator)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Alignment")
  {
    TestAlignmentHelper<NonAlignedVector>(PLASMA_ALIGNMENT_MINIMUM);
    TestAlignmentHelper<AlignedVector>(16);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "LargeBlockAllocator")
  {
    enum
    {
      BLOCK_SIZE_IN_BYTES = 4096 * 4
    };
    const plUInt32 uiPageSize = plSystemInformation::Get().GetMemoryPageSize();

    plLargeBlockAllocator<BLOCK_SIZE_IN_BYTES> allocator("Test", plFoundation::GetDefaultAllocator(), plMemoryTrackingFlags::EnableAllocationTracking);

    plDynamicArray<plDataBlock<int, BLOCK_SIZE_IN_BYTES>> blocks;
    blocks.Reserve(1000);

    for (plUInt32 i = 0; i < 17; ++i)
    {
      auto block = allocator.AllocateBlock<int>();
      PLASMA_TEST_BOOL(plMemoryUtils::IsAligned(block.m_pData, uiPageSize)); // test page alignment
      PLASMA_TEST_INT(block.m_uiCount, 0);

      blocks.PushBack(block);
    }

    plAllocatorBase::Stats stats = allocator.GetStats();

    PLASMA_TEST_BOOL(stats.m_uiNumAllocations == 17);
    PLASMA_TEST_BOOL(stats.m_uiNumDeallocations == 0);
    PLASMA_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (plUInt32 i = 0; i < 200; ++i)
    {
      auto block = allocator.AllocateBlock<int>();
      blocks.PushBack(block);
    }

    for (plUInt32 i = 0; i < 200; ++i)
    {
      allocator.DeallocateBlock(blocks.PeekBack());
      blocks.PopBack();
    }

    stats = allocator.GetStats();

    PLASMA_TEST_BOOL(stats.m_uiNumAllocations == 217);
    PLASMA_TEST_BOOL(stats.m_uiNumDeallocations == 200);
    PLASMA_TEST_BOOL(stats.m_uiAllocationSize == 17 * BLOCK_SIZE_IN_BYTES);

    for (plUInt32 i = 0; i < 2000; ++i)
    {
      plUInt32 uiAction = rand() % 2;
      if (uiAction == 0)
      {
        blocks.PushBack(allocator.AllocateBlock<int>());
      }
      else if (blocks.GetCount() > 0)
      {
        plUInt32 uiIndex = rand() % blocks.GetCount();
        auto block = blocks[uiIndex];

        allocator.DeallocateBlock(block);

        blocks.RemoveAtAndSwap(uiIndex);
      }
    }

    for (plUInt32 i = 0; i < blocks.GetCount(); ++i)
    {
      allocator.DeallocateBlock(blocks[i]);
    }

    stats = allocator.GetStats();

    PLASMA_TEST_BOOL(stats.m_uiNumAllocations - stats.m_uiNumDeallocations == 0);
    PLASMA_TEST_BOOL(stats.m_uiAllocationSize == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StackAllocator")
  {
    plStackAllocator<> allocator("TestStackAllocator", plFoundation::GetAlignedAllocator());

    void* blocks[8];
    for (size_t i = 0; i < PLASMA_ARRAY_SIZE(blocks); i++)
    {
      size_t size = i + 1;
      blocks[i] = allocator.Allocate(size, sizeof(void*), nullptr);
      PLASMA_TEST_BOOL(blocks[i] != nullptr);
      if (i > 0)
      {
        PLASMA_TEST_BOOL((plUInt8*)blocks[i - 1] + (size - 1) <= blocks[i]);
      }
    }

    for (size_t i = PLASMA_ARRAY_SIZE(blocks); i--;)
    {
      allocator.Deallocate(blocks[i]);
    }

    size_t sizes[] = {128, 128, 4096, 1024, 1024, 16000, 512, 512, 768, 768, 16000, 16000, 16000, 16000};
    void* allocs[PLASMA_ARRAY_SIZE(sizes)];
    for (size_t i = 0; i < PLASMA_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      PLASMA_TEST_BOOL(allocs[i] != nullptr);
    }
    for (size_t i = PLASMA_ARRAY_SIZE(sizes); i--;)
    {
      allocator.Deallocate(allocs[i]);
    }
    allocator.Reset();

    for (size_t i = 0; i < PLASMA_ARRAY_SIZE(sizes); i++)
    {
      allocs[i] = allocator.Allocate(sizes[i], sizeof(void*), nullptr);
      PLASMA_TEST_BOOL(allocs[i] != nullptr);
    }
    allocator.Reset();
    allocs[0] = allocator.Allocate(8, sizeof(void*), nullptr);
    PLASMA_TEST_BOOL(allocs[0] < allocs[1]);
    allocator.Deallocate(allocs[0]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "StackAllocator with non-PODs")
  {
    plStackAllocator<> allocator("TestStackAllocator", plFoundation::GetAlignedAllocator());

    plDynamicArray<plConstructionCounter*> counters;
    counters.Reserve(100);

    for (plUInt32 i = 0; i < 100; ++i)
    {
      counters.PushBack(PLASMA_NEW(&allocator, plConstructionCounter));
    }

    for (plUInt32 i = 0; i < 100; ++i)
    {
      PLASMA_NEW(&allocator, NonAlignedVector);
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasConstructed(100));

    for (plUInt32 i = 0; i < 50; ++i)
    {
      PLASMA_DELETE(&allocator, counters[i * 2]);
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasDestructed(50));

    allocator.Reset();

    PLASMA_TEST_BOOL(plConstructionCounter::HasDestructed(50));
  }
}
