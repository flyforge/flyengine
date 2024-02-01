#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

struct PL_CORE_DLL plMsgDeleteGameObject : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgDeleteGameObject, plMessage);

  /// \brief If set to true, any parent/ancestor that has no other children or components will also be deleted.
  bool m_bDeleteEmptyParents = true;

  /// \brief This is used by plOnComponentFinishedAction to orchestrate when an object shall really be deleted.
  bool m_bCancel = false;
};
