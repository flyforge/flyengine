#pragma once

#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>

template <typename ElemType>
class ArrayPtrTask final : public plTask
{
public:
  ArrayPtrTask(plArrayPtr<ElemType> payload, plParallelForFunction<ElemType> taskCallback, plUInt32 uiItemsPerInvocation)
    : m_Payload(payload)
    , m_uiItemsPerInvocation(uiItemsPerInvocation)
    , m_TaskCallback(std::move(taskCallback))
  {
  }

  void Execute() override
  {
    // Work through all of them.
    m_TaskCallback(0, m_Payload);
  }

  void ExecuteWithMultiplicity(plUInt32 uiInvocation) const override
  {
    const plUInt32 uiSliceStartIndex = uiInvocation * m_uiItemsPerInvocation;

    const plUInt32 uiRemainingItems = uiSliceStartIndex > m_Payload.GetCount() ? 0 : m_Payload.GetCount() - uiSliceStartIndex;
    const plUInt32 uiSliceItemCount = plMath::Min(m_uiItemsPerInvocation, uiRemainingItems);

    if (uiSliceItemCount > 0)
    {
      // Run through the calculated slice.
      auto taskItemSlice = m_Payload.GetSubArray(uiSliceStartIndex, uiSliceItemCount);
      m_TaskCallback(uiSliceStartIndex, taskItemSlice);
    }
  }

private:
  plArrayPtr<ElemType> m_Payload;
  plUInt32 m_uiItemsPerInvocation;
  plParallelForFunction<ElemType> m_TaskCallback;
};

template <typename ElemType>
void plTaskSystem::ParallelForInternal(plArrayPtr<ElemType> taskItems, plParallelForFunction<ElemType> taskCallback, const char* taskName, const plParallelForParams& params)
{
  if (taskItems.GetCount() <= params.m_uiBinSize)
  {
    ArrayPtrTask<ElemType> arrayPtrTask(taskItems, std::move(taskCallback), taskItems.GetCount());
    arrayPtrTask.ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    PL_PROFILE_SCOPE(arrayPtrTask.m_sTaskName);
    arrayPtrTask.Execute();
  }
  else
  {
    plUInt32 uiMultiplicity;
    plUInt64 uiItemsPerInvocation;
    params.DetermineThreading(taskItems.GetCount(), uiMultiplicity, uiItemsPerInvocation);

    plAllocator* pAllocator = (params.m_pTaskAllocator != nullptr) ? params.m_pTaskAllocator : plFoundation::GetDefaultAllocator();

    plSharedPtr<ArrayPtrTask<ElemType>> pArrayPtrTask = PL_NEW(pAllocator, ArrayPtrTask<ElemType>, taskItems, std::move(taskCallback), static_cast<plUInt32>(uiItemsPerInvocation));
    pArrayPtrTask->ConfigureTask(taskName ? taskName : "Generic ArrayPtr Task", params.m_NestingMode);

    pArrayPtrTask->SetMultiplicity(uiMultiplicity);
    plTaskGroupID taskGroupId = plTaskSystem::StartSingleTask(pArrayPtrTask, plTaskPriority::EarlyThisFrame);
    plTaskSystem::WaitForGroup(taskGroupId);
  }
}

template <typename ElemType, typename Callback>
void plTaskSystem::ParallelFor(plArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const plParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](
                           plUInt32 /*uiBaseIndex*/, plArrayPtr<ElemType> taskSlice) { taskCallback(taskSlice); };

  ParallelForInternal<ElemType>(
    taskItems, plParallelForFunction<ElemType>(std::move(wrappedCallback), plFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void plTaskSystem::ParallelForSingle(plArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const plParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](plUInt32 /*uiBaseIndex*/, plArrayPtr<ElemType> taskSlice) {
    // Handing in by non-const& allows to use callbacks with (non-)const& as well as value parameters.
    for (ElemType& taskItem : taskSlice)
    {
      taskCallback(taskItem);
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, plParallelForFunction<ElemType>(std::move(wrappedCallback), plFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}

template <typename ElemType, typename Callback>
void plTaskSystem::ParallelForSingleIndex(
  plArrayPtr<ElemType> taskItems, Callback taskCallback, const char* szTaskName, const plParallelForParams& params)
{
  auto wrappedCallback = [taskCallback = std::move(taskCallback)](plUInt32 uiBaseIndex, plArrayPtr<ElemType> taskSlice) {
    for (plUInt32 uiIndex = 0; uiIndex < taskSlice.GetCount(); ++uiIndex)
    {
      // Handing in by dereferenced pointer allows to use callbacks with (non-)const& as well as value parameters.
      taskCallback(uiBaseIndex + uiIndex, *(taskSlice.GetPtr() + uiIndex));
    }
  };

  ParallelForInternal<ElemType>(
    taskItems, plParallelForFunction<ElemType>(std::move(wrappedCallback), plFrameAllocator::GetCurrentAllocator()), szTaskName, params);
}
