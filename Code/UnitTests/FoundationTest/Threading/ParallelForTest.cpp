#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Threading/TaskSystem.h>

namespace
{
  static constexpr plUInt32 s_uiNumberOfWorkers = 4;
  static constexpr plUInt32 s_uiTaskItemSliceSize = 25;
  static constexpr plUInt32 s_uiTotalNumberOfTaskItems = s_uiNumberOfWorkers * s_uiTaskItemSliceSize;
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Threading, ParallelFor)
{
  // set up controlled task system environment
  plTaskSystem::SetWorkerThreadCount(::s_uiNumberOfWorkers, ::s_uiNumberOfWorkers);

  // shared variables
  plMutex dataAccessMutex;

  plUInt32 uiRangesEncounteredCheck = 0;
  plUInt32 uiNumbersSum = 0;

  plUInt32 uiNumbersCheckSum = 0;
  plStaticArray<plUInt32, ::s_uiTotalNumberOfTaskItems> numbers;

  plParallelForParams parallelForParams;
  parallelForParams.m_uiBinSize = ::s_uiTaskItemSliceSize;
  parallelForParams.m_uiMaxTasksPerThread = 1;

  auto ResetSharedVariables = [&uiRangesEncounteredCheck, &uiNumbersSum, &uiNumbersCheckSum, &numbers]() {
    uiRangesEncounteredCheck = 0;
    uiNumbersSum = 0;

    uiNumbersCheckSum = 0;

    numbers.EnsureCount(::s_uiTotalNumberOfTaskItems);
    for (plUInt32 i = 0; i < ::s_uiTotalNumberOfTaskItems; ++i)
    {
      numbers[i] = i + 1;
      uiNumbersCheckSum += numbers[i];
    }
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Parallel For (Indexed)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of number assigned to us via index ranges and
    // check if the ranges described by them are as expected
    plTaskSystem::ParallelForIndexed(
      0, ::s_uiTotalNumberOfTaskItems,
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &numbers](plUInt32 uiStartIndex, plUInt32 uiEndIndex) {
        PLASMA_LOCK(dataAccessMutex);

        // size check
        PLASMA_TEST_INT(uiEndIndex - uiStartIndex, ::s_uiTaskItemSliceSize);

        // note down which range this is
        uiRangesEncounteredCheck |= 1 << (uiStartIndex / ::s_uiTaskItemSliceSize);

        // sum up numbers in our slice
        for (plUInt32 uiIndex = uiStartIndex; uiIndex < uiEndIndex; ++uiIndex)
        {
          uiNumbersSum += numbers[uiIndex];
        }
      },
      "ParallelForIndexed Test", parallelForParams);

    // check results
    PLASMA_TEST_INT(uiRangesEncounteredCheck, 0b1111);
    PLASMA_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Parallel For (Array)")
  {
    // reset
    ResetSharedVariables();

    // test-specific data
    plStaticArray<plUInt32*, ::s_uiNumberOfWorkers> startAddresses;
    for (plUInt32 i = 0; i < ::s_uiNumberOfWorkers; ++i)
    {
      startAddresses.PushBack(numbers.GetArrayPtr().GetPtr() + (i * ::s_uiTaskItemSliceSize));
    }

    // test
    // sum up the slice of numbers assigned to us via array pointers and
    // check if the ranges described by them are as expected
    plTaskSystem::ParallelFor<plUInt32>(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiRangesEncounteredCheck, &uiNumbersSum, &startAddresses](plArrayPtr<plUInt32> taskItemSlice) {
        PLASMA_LOCK(dataAccessMutex);

        // size check
        PLASMA_TEST_INT(taskItemSlice.GetCount(), ::s_uiTaskItemSliceSize);

        // note down which range this is
        for (plUInt32 index = 0; index < startAddresses.GetCount(); ++index)
        {
          if (startAddresses[index] == taskItemSlice.GetPtr())
          {
            uiRangesEncounteredCheck |= 1 << index;
          }
        }

        // sum up numbers in our slice
        for (const plUInt32& number : taskItemSlice)
        {
          uiNumbersSum += number;
        }
      },
      "ParallelFor Array Test", parallelForParams);

    // check results
    PLASMA_TEST_INT(15, 0b1111);
    PLASMA_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Parallel For (Array, Single)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of numbers by summing up the individual numbers that get handed to us
    plTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](plUInt32 uiNumber) {
        PLASMA_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Test", parallelForParams);

    // check the resulting sum
    PLASMA_TEST_INT(uiNumbersSum, uiNumbersCheckSum);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Parallel For (Array, Single, Index)")
  {
    // reset
    ResetSharedVariables();

    // test
    // sum up the slice of numbers that got assigned to us via an index range
    plTaskSystem::ParallelForSingleIndex(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](plUInt32 uiIndex, plUInt32 uiNumber) {
        PLASMA_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber + (uiIndex + 1);
      },
      "ParallelFor Array Single Index Test", parallelForParams);

    // check the resulting sum
    PLASMA_TEST_INT(uiNumbersSum, 2 * uiNumbersCheckSum);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Parallel For (Array, Single) Write")
  {
    // reset
    ResetSharedVariables();

    // test
    // modify the original array of numbers
    plTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex](plUInt32& ref_uiNumber) {
        PLASMA_LOCK(dataAccessMutex);
        ref_uiNumber = ref_uiNumber * 3;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // sum up the new values to test if writing worked
    plTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const plUInt32& uiNumber) {
        PLASMA_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // check the resulting sum
    PLASMA_TEST_INT(uiNumbersSum, 3 * uiNumbersCheckSum);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Parallel For (Array, Single, Index) Write")
  {
    // reset
    ResetSharedVariables();

    // test
    // modify the original array of numbers
    plTaskSystem::ParallelForSingleIndex(
      numbers.GetArrayPtr(),
      [&dataAccessMutex](plUInt32, plUInt32& ref_uiNumber) {
        PLASMA_LOCK(dataAccessMutex);
        ref_uiNumber = ref_uiNumber * 4;
      },
      "ParallelFor Array Single Write Test (Write)", parallelForParams);

    // sum up the new values to test if writing worked
    plTaskSystem::ParallelForSingle(
      numbers.GetArrayPtr(),
      [&dataAccessMutex, &uiNumbersSum](const plUInt32& uiNumber) {
        PLASMA_LOCK(dataAccessMutex);
        uiNumbersSum += uiNumber;
      },
      "ParallelFor Array Single Write Test (Sum)", parallelForParams);

    // check the resulting sum
    PLASMA_TEST_INT(uiNumbersSum, 4 * uiNumbersCheckSum);
  }
}
