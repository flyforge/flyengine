#pragma once

#include <Foundation/Communication/Message.h>

// BEGIN-DOCS-CODE-SNIPPET: message-decl
struct plMsgSetText : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgSetText, plMessage);

  plString m_sText;
};
// END-DOCS-CODE-SNIPPET
