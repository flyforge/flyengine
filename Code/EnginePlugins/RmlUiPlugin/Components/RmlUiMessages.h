#pragma once

#include <Foundation/Communication/Message.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct PL_RMLUIPLUGIN_DLL plMsgRmlUiReload : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgRmlUiReload, plMessage);
};
