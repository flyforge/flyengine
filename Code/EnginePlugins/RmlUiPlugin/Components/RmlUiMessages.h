#pragma once

#include <Foundation/Communication/Message.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct PLASMA_RMLUIPLUGIN_DLL plMsgRmlUiReload : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgRmlUiReload, plMessage);
};
