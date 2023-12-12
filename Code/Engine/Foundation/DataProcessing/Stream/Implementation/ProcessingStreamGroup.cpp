#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/MemoryUtils.h>

plProcessingStreamGroup::plProcessingStreamGroup()
{
  Clear();
}

plProcessingStreamGroup::~plProcessingStreamGroup()
{
  Clear();
}

void plProcessingStreamGroup::Clear()
{
  ClearProcessors();

  m_uiPendingNumberOfElementsToSpawn = 0;
  m_uiNumElements = 0;
  m_uiNumActiveElements = 0;
  m_uiHighestNumActiveElements = 0;
  m_bStreamAssignmentDirty = true;

  for (plProcessingStream* pStream : m_DataStreams)
  {
    PLASMA_DEFAULT_DELETE(pStream);
  }

  m_DataStreams.Clear();
}

void plProcessingStreamGroup::AddProcessor(plProcessingStreamProcessor* pProcessor)
{
  PLASMA_ASSERT_DEV(pProcessor != nullptr, "Stream processor may not be null!");

  if (pProcessor->m_pStreamGroup != nullptr)
  {
    plLog::Debug("Stream processor is already assigned to a stream group!");
    return;
  }

  m_Processors.PushBack(pProcessor);

  pProcessor->m_pStreamGroup = this;

  m_bStreamAssignmentDirty = true;
}

void plProcessingStreamGroup::RemoveProcessor(plProcessingStreamProcessor* pProcessor)
{
  m_Processors.RemoveAndCopy(pProcessor);
  pProcessor->GetDynamicRTTI()->GetAllocator()->Deallocate(pProcessor);
}

void plProcessingStreamGroup::ClearProcessors()
{
  m_bStreamAssignmentDirty = true;

  for (plProcessingStreamProcessor* pProcessor : m_Processors)
  {
    pProcessor->GetDynamicRTTI()->GetAllocator()->Deallocate(pProcessor);
  }

  m_Processors.Clear();
}

plProcessingStream* plProcessingStreamGroup::AddStream(plStringView sName, plProcessingStream::DataType type)
{
  // Treat adding a stream two times as an error (return null)
  if (GetStreamByName(sName))
    return nullptr;

  plHashedString Name;
  Name.Assign(sName);
  plProcessingStream* pStream = PLASMA_DEFAULT_NEW(plProcessingStream, Name, type, plProcessingStream::GetDataTypeSize(type), 16);

  m_DataStreams.PushBack(pStream);

  m_bStreamAssignmentDirty = true;

  return pStream;
}

void plProcessingStreamGroup::RemoveStreamByName(plStringView sName)
{
  plHashedString Name;
  Name.Assign(sName);

  for (plUInt32 i = 0; i < m_DataStreams.GetCount(); ++i)
  {
    if (m_DataStreams[i]->GetName() == Name)
    {
      PLASMA_DEFAULT_DELETE(m_DataStreams[i]);
      m_DataStreams.RemoveAtAndSwap(i);

      m_bStreamAssignmentDirty = true;
      break;
    }
  }
}

plProcessingStream* plProcessingStreamGroup::GetStreamByName(plStringView sName) const
{
  plHashedString Name;
  Name.Assign(sName);

  for (plProcessingStream* Stream : m_DataStreams)
  {
    if (Stream->GetName() == Name)
    {
      return Stream;
    }
  }

  return nullptr;
}

void plProcessingStreamGroup::SetSize(plUInt64 uiNumElements)
{
  if (m_uiNumElements == uiNumElements)
    return;

  m_uiNumElements = uiNumElements;

  // Also reset any pending remove and spawn operations since they refer to the old size and content
  m_PendingRemoveIndices.Clear();
  m_uiPendingNumberOfElementsToSpawn = 0;

  m_uiHighestNumActiveElements = 0;

  // Stream processors etc. may have pointers to the stream data for some reason.
  m_bStreamAssignmentDirty = true;
}

/// \brief Removes an element (e.g. due to the death of a particle etc.), this will be enqueued (and thus is safe to be called from within data
/// processors).
void plProcessingStreamGroup::RemoveElement(plUInt64 uiElementIndex)
{
  if (m_PendingRemoveIndices.Contains(uiElementIndex))
    return;

  PLASMA_ASSERT_DEBUG(uiElementIndex < m_uiNumActiveElements, "Element which should be removed is outside of active element range!");

  m_PendingRemoveIndices.PushBack(uiElementIndex);
}

/// \brief Spawns a number of new elements, they will be added as newly initialized stream elements. Safe to call from data processors since the
/// spawning will be queued.
void plProcessingStreamGroup::InitializeElements(plUInt64 uiNumElements)
{
  m_uiPendingNumberOfElementsToSpawn += uiNumElements;
}

void plProcessingStreamGroup::Process()
{
  EnsureStreamAssignmentValid();

  // TODO: Identify which processors work on which streams and find independent groups and use separate tasks for them?
  for (plProcessingStreamProcessor* pStreamProcessor : m_Processors)
  {
    pStreamProcessor->Process(m_uiNumActiveElements);
  }

  // Run any pending deletions which happened due to stream processor execution
  RunPendingDeletions();

  // spawning here (instead of before processing) allows for particles to exist for exactly one frame
  // they will be created, initialized, then rendered, and the next Process() will already delete them
  RunPendingSpawns();
}


void plProcessingStreamGroup::RunPendingDeletions()
{
  plStreamGroupElementRemovedEvent e;
  e.m_pStreamGroup = this;

  // Remove elements
  while (!m_PendingRemoveIndices.IsEmpty())
  {
    if (m_uiNumActiveElements == 0)
      break;

    const plUInt64 uiLastActiveElementIndex = m_uiNumActiveElements - 1;

    const plUInt64 uiElementToRemove = m_PendingRemoveIndices.PeekBack();
    m_PendingRemoveIndices.PopBack();

    PLASMA_ASSERT_DEBUG(uiElementToRemove < m_uiNumActiveElements, "Invalid index to remove");

    // inform any interested party about the tragic death
    e.m_uiElementIndex = uiElementToRemove;
    m_ElementRemovedEvent.Broadcast(e);

    // If the element which should be removed is the last element we can just decrement the number of active elements
    // and no further work needs to be done
    if (uiElementToRemove == uiLastActiveElementIndex)
    {
      m_uiNumActiveElements--;
      continue;
    }

    // Since we swap with the last element we need to make sure that any pending removals of the (current) last element are updated
    // and point to the place where we moved the data to.
    for (plUInt32 i = 0; i < m_PendingRemoveIndices.GetCount(); ++i)
    {
      // Is the pending remove in the array actually the last element we use to swap with? It's simply a matter of updating it to point to the new
      // index.
      if (m_PendingRemoveIndices[i] == uiLastActiveElementIndex)
      {
        m_PendingRemoveIndices[i] = uiElementToRemove;

        // We can break since the RemoveElement() operation takes care that each index can be in the array only once
        break;
      }
    }

    // Move the data
    for (plProcessingStream* pStream : m_DataStreams)
    {
      const plUInt64 uiStreamElementStride = pStream->GetElementStride();
      const plUInt64 uiStreamElementSize = pStream->GetElementSize();
      const void* pSourceData = plMemoryUtils::AddByteOffset(pStream->GetData(), static_cast<ptrdiff_t>(uiLastActiveElementIndex * uiStreamElementStride));
      void* pTargetData = plMemoryUtils::AddByteOffset(pStream->GetWritableData(), static_cast<ptrdiff_t>(uiElementToRemove * uiStreamElementStride));

      plMemoryUtils::Copy<plUInt8>(static_cast<plUInt8*>(pTargetData), static_cast<const plUInt8*>(pSourceData), static_cast<size_t>(uiStreamElementSize));
    }

    // And decrease the size since we swapped the last element to the location of the element we just removed
    m_uiNumActiveElements--;
  }

  m_PendingRemoveIndices.Clear();
}

void plProcessingStreamGroup::EnsureStreamAssignmentValid()
{
  // If any stream processors or streams were added we may need to inform them.
  if (m_bStreamAssignmentDirty)
  {
    SortProcessorsByPriority();

    // Set the new size on all stream.
    for (plProcessingStream* Stream : m_DataStreams)
    {
      Stream->SetSize(m_uiNumElements);
    }

    for (plProcessingStreamProcessor* pStreamProcessor : m_Processors)
    {
      pStreamProcessor->UpdateStreamBindings().IgnoreResult();
    }

    m_bStreamAssignmentDirty = false;
  }
}

void plProcessingStreamGroup::RunPendingSpawns()
{
  // Check if elements need to be spawned. If this is the case spawn them. (This is limited by the maximum number of elements).
  if (m_uiPendingNumberOfElementsToSpawn > 0)
  {
    m_uiPendingNumberOfElementsToSpawn = plMath::Min(m_uiPendingNumberOfElementsToSpawn, m_uiNumElements - m_uiNumActiveElements);

    if (m_uiPendingNumberOfElementsToSpawn)
    {
      for (plProcessingStreamProcessor* pSpawner : m_Processors)
      {
        pSpawner->InitializeElements(m_uiNumActiveElements, m_uiPendingNumberOfElementsToSpawn);
      }
    }

    m_uiNumActiveElements += m_uiPendingNumberOfElementsToSpawn;

    m_uiHighestNumActiveElements = plMath::Max(m_uiNumActiveElements, m_uiHighestNumActiveElements);

    m_uiPendingNumberOfElementsToSpawn = 0;
  }
}

struct ProcessorComparer
{
  PLASMA_ALWAYS_INLINE bool Less(const plProcessingStreamProcessor* a, const plProcessingStreamProcessor* b) const { return a->m_fPriority < b->m_fPriority; }
};

void plProcessingStreamGroup::SortProcessorsByPriority()
{
  ProcessorComparer cmp;
  m_Processors.Sort(cmp);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStreamGroup);
