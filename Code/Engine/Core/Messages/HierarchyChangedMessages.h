#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

struct PL_CORE_DLL plMsgParentChanged : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgParentChanged, plMessage);

  enum class Type
  {
    ParentLinked,
    ParentUnlinked,
    Invalid
  };

  Type m_Type = Type::Invalid;
  plGameObjectHandle m_hParent; // previous or new parent, depending on m_Type
};

struct PL_CORE_DLL plMsgChildrenChanged : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgChildrenChanged, plMessage);

  enum class Type
  {
    ChildAdded,
    ChildRemoved
  };

  Type m_Type;
  plGameObjectHandle m_hParent;
  plGameObjectHandle m_hChild;
};

struct PL_CORE_DLL plMsgComponentsChanged : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgComponentsChanged, plMessage);

  enum class Type
  {
    ComponentAdded,
    ComponentRemoved,
    Invalid
  };

  Type m_Type = Type::Invalid;
  plGameObjectHandle m_hOwner;
  plComponentHandle m_hComponent;
};
