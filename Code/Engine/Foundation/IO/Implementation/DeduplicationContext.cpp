#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>

PL_IMPLEMENT_SERIALIZATION_CONTEXT(plDeduplicationReadContext);

plDeduplicationReadContext::plDeduplicationReadContext() = default;
plDeduplicationReadContext::~plDeduplicationReadContext() = default;

//////////////////////////////////////////////////////////////////////////

PL_IMPLEMENT_SERIALIZATION_CONTEXT(plDeduplicationWriteContext);

plDeduplicationWriteContext::plDeduplicationWriteContext() = default;
plDeduplicationWriteContext::~plDeduplicationWriteContext() = default;


