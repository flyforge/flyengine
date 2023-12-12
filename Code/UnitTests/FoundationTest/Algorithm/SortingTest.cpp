#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>

namespace
{
  struct CustomComparer
  {
    PLASMA_ALWAYS_INLINE bool Less(plInt32 a, plInt32 b) const { return a > b; }

    // Comparision via operator. Sorting algorithm should prefer Less operator
    bool operator()(plInt32 a, plInt32 b) const { return a < b; }
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Algorithm, Sorting)
{
  plDynamicArray<plInt32> a1;

  for (plUInt32 i = 0; i < 2000; ++i)
  {
    a1.PushBack(rand() % 100000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "QuickSort")
  {
    plDynamicArray<plInt32> a2 = a1;

    plSorting::QuickSort(a1, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

    for (plUInt32 i = 1; i < a1.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(a1[i - 1] >= a1[i]);
    }

    plArrayPtr<plInt32> arrayPtr = a2;
    plSorting::QuickSort(arrayPtr, CustomComparer()); // quicksort uses insertion sort for partitions smaller than 16 elements

    for (plUInt32 i = 1; i < arrayPtr.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(arrayPtr[i - 1] >= arrayPtr[i]);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "QuickSort - Lambda")
  {
    plDynamicArray<plInt32> a2 = a1;
    plSorting::QuickSort(a2, [](const auto& a, const auto& b) { return a > b; });

    for (plUInt32 i = 1; i < a2.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(a2[i - 1] >= a2[i]);
    }
  }
}
