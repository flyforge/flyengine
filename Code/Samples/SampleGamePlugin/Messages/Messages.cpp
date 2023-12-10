#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: message-impl
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgSetText);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetText, 1, plRTTIDefaultAllocator<plMsgSetText>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// END-DOCS-CODE-SNIPPET
// clang-format on
