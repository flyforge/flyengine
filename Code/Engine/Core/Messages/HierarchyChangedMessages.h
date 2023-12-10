#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct PLASMA_CORE_DLL plMsgParentChanged : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgParentChanged, plMessage);

  enum class Type
  {
    ParentLinked,
    ParentUnlinked,
  };

  Type m_Type;
  plGameObjectHandle m_hParent; // previous or new parent, depending on m_Type
};

struct PLASMA_CORE_DLL plMsgChildrenChanged : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgChildrenChanged, plMessage);

  enum class Type
  {
    ChildAdded,
    ChildRemoved
  };

  Type m_Type;
  plGameObjectHandle m_hParent;
  plGameObjectHandle m_hChild;
};

struct PLASMA_CORE_DLL plMsgComponentsChanged : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgComponentsChanged, plMessage);

  enum class Type
  {
    ComponentAdded,
    ComponentRemoved
  };

  Type m_Type;
  plGameObjectHandle m_hOwner;
  plComponentHandle m_hComponent;
};
