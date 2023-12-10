#include <Foundation/FoundationPCH.h>

#include <Foundation/Basics.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamProcessor.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcessingStreamProcessor, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plProcessingStreamProcessor::plProcessingStreamProcessor()

  = default;

plProcessingStreamProcessor::~plProcessingStreamProcessor()
{
  m_pStreamGroup = nullptr;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_DataProcessing_Stream_Implementation_ProcessingStreamProcessor);
