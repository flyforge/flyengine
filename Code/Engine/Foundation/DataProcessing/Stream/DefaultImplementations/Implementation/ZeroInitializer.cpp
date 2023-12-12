#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/Memory/MemoryUtils.h>

#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>

#include <Foundation/DataProcessing/Stream/DefaultImplementations/ZeroInitializer.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcessingStreamSpawnerZeroInitialized, 1, plRTTIDefaultAllocator<plProcessingStreamSpawnerZeroInitialized>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plProcessingStreamSpawnerZeroInitialized::plProcessingStreamSpawnerZeroInitialized()

  = default;

void plProcessingStreamSpawnerZeroInitialized::SetStreamName(plStringView sStreamName)
{
  m_sStreamName.Assign(sStreamName);
}

plResult plProcessingStreamSpawnerZeroInitialized::UpdateStreamBindings()
{
  PLASMA_ASSERT_DEBUG(!m_sStreamName.IsEmpty(), "plProcessingStreamSpawnerZeroInitialized: Stream name has not been configured");

  m_pStream = m_pStreamGroup->GetStreamByName(m_sStreamName);
  return m_pStream ? PLASMA_SUCCESS : PLASMA_FAILURE;
}


void plProcessingStreamSpawnerZeroInitialized::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  const plUInt64 uiElementSize = m_pStream->GetElementSize();
  const plUInt64 uiElementStride = m_pStream->GetElementStride();

  for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    plMemoryUtils::ZeroFill<plUInt8>(
      static_cast<plUInt8*>(plMemoryUtils::AddByteOffset(m_pStream->GetWritableData(), static_cast<ptrdiff_t>(i * uiElementStride))),
      static_cast<size_t>(uiElementSize));
  }
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_DefaultImplementations_Implementation_ZeroInitializer);
