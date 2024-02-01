#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct PL_CORE_DLL plMsgOnlyApplyToObject : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgOnlyApplyToObject, plMessage);

  plGameObjectHandle m_hObject;
};
