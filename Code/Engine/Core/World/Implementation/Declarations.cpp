#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/World/World.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGameObjectHandle, plNoBase, 1, plRTTIDefaultAllocator<plGameObjectHandle>)
PLASMA_END_STATIC_REFLECTED_TYPE;
PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plGameObjectHandle);

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plComponentHandle, plNoBase, 1, plRTTIDefaultAllocator<plComponentHandle>)
PLASMA_END_STATIC_REFLECTED_TYPE;
PLASMA_DEFINE_CUSTOM_VARIANT_TYPE(plComponentHandle);

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plObjectMode, 1)
  PLASMA_ENUM_CONSTANTS(plObjectMode::Automatic, plObjectMode::ForceDynamic)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plOnComponentFinishedAction, 1)
  PLASMA_ENUM_CONSTANTS(plOnComponentFinishedAction::None, plOnComponentFinishedAction::DeleteComponent, plOnComponentFinishedAction::DeleteGameObject)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plOnComponentFinishedAction2, 1)
  PLASMA_ENUM_CONSTANTS(plOnComponentFinishedAction2::None, plOnComponentFinishedAction2::DeleteComponent, plOnComponentFinishedAction2::DeleteGameObject, plOnComponentFinishedAction2::Restart)
PLASMA_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

void operator<<(plStreamWriter& Stream, const plGameObjectHandle& Value)
{
  PLASMA_ASSERT_DEV(false, "This function should not be called. Use plWorldWriter::WriteGameObjectHandle instead.");
}

void operator>>(plStreamReader& Stream, plGameObjectHandle& Value)
{
  PLASMA_ASSERT_DEV(false, "This function should not be called. Use plWorldReader::ReadGameObjectHandle instead.");
}

void operator<<(plStreamWriter& Stream, const plComponentHandle& Value)
{
  PLASMA_ASSERT_DEV(false, "This function should not be called. Use plWorldWriter::WriteComponentHandle instead.");
}

void operator>>(plStreamReader& Stream, plComponentHandle& Value)
{
  PLASMA_ASSERT_DEV(false, "This function should not be called. Use plWorldReader::ReadComponentHandle instead.");
}

//////////////////////////////////////////////////////////////////////////

namespace
{
  template <typename T>
  void HandleFinishedActionImpl(plComponent* pComponent, typename T::Enum action)
  {
    if (action == T::DeleteGameObject)
    {
      // Send a message to the owner object to check whether another component wants to delete this object later.
      // Can't use plGameObject::SendMessage because the object would immediately delete itself and furthermore the sender component needs to be
      // filtered out here.
      plMsgDeleteGameObject msg;

      for (plComponent* pComp : pComponent->GetOwner()->GetComponents())
      {
        if (pComp == pComponent)
          continue;

        pComp->SendMessage(msg);
        if (msg.m_bCancel)
        {
          action = T::DeleteComponent;
          break;
        }
      }

      if (action == T::DeleteGameObject)
      {
        pComponent->GetWorld()->DeleteObjectDelayed(pComponent->GetOwner()->GetHandle());
        return;
      }
    }

    if (action == T::DeleteComponent)
    {
      pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle());
    }
  }

  template <typename T>
  void HandleDeleteObjectMsgImpl(plMsgDeleteGameObject& msg, plEnum<T>& action)
  {
    if (action == T::DeleteComponent)
    {
      msg.m_bCancel = true;
      action = T::DeleteGameObject;
    }
    else if (action == T::DeleteGameObject)
    {
      msg.m_bCancel = true;
    }
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

void plOnComponentFinishedAction::HandleFinishedAction(plComponent* pComponent, plOnComponentFinishedAction::Enum action)
{
  HandleFinishedActionImpl<plOnComponentFinishedAction>(pComponent, action);
}

void plOnComponentFinishedAction::HandleDeleteObjectMsg(plMsgDeleteGameObject& msg, plEnum<plOnComponentFinishedAction>& action)
{
  HandleDeleteObjectMsgImpl(msg, action);
}

//////////////////////////////////////////////////////////////////////////

void plOnComponentFinishedAction2::HandleFinishedAction(plComponent* pComponent, plOnComponentFinishedAction2::Enum action)
{
  HandleFinishedActionImpl<plOnComponentFinishedAction2>(pComponent, action);
}

void plOnComponentFinishedAction2::HandleDeleteObjectMsg(plMsgDeleteGameObject& msg, plEnum<plOnComponentFinishedAction2>& action)
{
  HandleDeleteObjectMsgImpl(msg, action);
}

PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_Declarations);
