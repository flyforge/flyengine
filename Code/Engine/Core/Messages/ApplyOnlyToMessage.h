#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct PLASMA_CORE_DLL plMsgOnlyApplyToObject : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgOnlyApplyToObject, plMessage);

  plGameObjectHandle m_hObject;
};
