#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>

PLASMA_IMPLEMENT_SERIALIZATION_CONTEXT(plDeduplicationReadContext);

plDeduplicationReadContext::plDeduplicationReadContext() = default;
plDeduplicationReadContext::~plDeduplicationReadContext() = default;

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_SERIALIZATION_CONTEXT(plDeduplicationWriteContext);

plDeduplicationWriteContext::plDeduplicationWriteContext() = default;
plDeduplicationWriteContext::~plDeduplicationWriteContext() = default;



PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_DeduplicationContext);
