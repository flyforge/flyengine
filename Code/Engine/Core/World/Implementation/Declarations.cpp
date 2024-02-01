#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/World/World.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plGameObjectHandle, plNoBase, 1, plRTTIDefaultAllocator<plGameObjectHandle>)
PL_END_STATIC_REFLECTED_TYPE;
PL_DEFINE_CUSTOM_VARIANT_TYPE(plGameObjectHandle);

PL_BEGIN_STATIC_REFLECTED_TYPE(plComponentHandle, plNoBase, 1, plRTTIDefaultAllocator<plComponentHandle>)
PL_END_STATIC_REFLECTED_TYPE;
PL_DEFINE_CUSTOM_VARIANT_TYPE(plComponentHandle);

PL_BEGIN_STATIC_REFLECTED_ENUM(plObjectMode, 1)
  PL_ENUM_CONSTANTS(plObjectMode::Automatic, plObjectMode::ForceDynamic)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plOnComponentFinishedAction, 1)
  PL_ENUM_CONSTANTS(plOnComponentFinishedAction::None, plOnComponentFinishedAction::DeleteComponent, plOnComponentFinishedAction::DeleteGameObject)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_STATIC_REFLECTED_ENUM(plOnComponentFinishedAction2, 1)
  PL_ENUM_CONSTANTS(plOnComponentFinishedAction2::None, plOnComponentFinishedAction2::DeleteComponent, plOnComponentFinishedAction2::DeleteGameObject, plOnComponentFinishedAction2::Restart)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

void operator<<(plStreamWriter& inout_stream, const plGameObjectHandle& hValue)
{
  PL_ASSERT_DEV(false, "This function should not be called. Use plWorldWriter::WriteGameObjectHandle instead.");
}

void operator>>(plStreamReader& inout_stream, plGameObjectHandle& ref_hValue)
{
  PL_ASSERT_DEV(false, "This function should not be called. Use plWorldReader::ReadGameObjectHandle instead.");
}

void operator<<(plStreamWriter& inout_stream, const plComponentHandle& hValue)
{
  PL_ASSERT_DEV(false, "This function should not be called. Use plWorldWriter::WriteComponentHandle instead.");
}

void operator>>(plStreamReader& inout_stream, plComponentHandle& ref_hValue)
{
  PL_ASSERT_DEV(false, "This function should not be called. Use plWorldReader::ReadComponentHandle instead.");
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
  void HandleDeleteObjectMsgImpl(plMsgDeleteGameObject& ref_msg, plEnum<T>& ref_action)
  {
    if (ref_action == T::DeleteComponent)
    {
      ref_msg.m_bCancel = true;
      ref_action = T::DeleteGameObject;
    }
    else if (ref_action == T::DeleteGameObject)
    {
      ref_msg.m_bCancel = true;
    }
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

void plOnComponentFinishedAction::HandleFinishedAction(plComponent* pComponent, plOnComponentFinishedAction::Enum action)
{
  HandleFinishedActionImpl<plOnComponentFinishedAction>(pComponent, action);
}

void plOnComponentFinishedAction::HandleDeleteObjectMsg(plMsgDeleteGameObject& ref_msg, plEnum<plOnComponentFinishedAction>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

//////////////////////////////////////////////////////////////////////////

void plOnComponentFinishedAction2::HandleFinishedAction(plComponent* pComponent, plOnComponentFinishedAction2::Enum action)
{
  HandleFinishedActionImpl<plOnComponentFinishedAction2>(pComponent, action);
}

void plOnComponentFinishedAction2::HandleDeleteObjectMsg(plMsgDeleteGameObject& ref_msg, plEnum<plOnComponentFinishedAction2>& ref_action)
{
  HandleDeleteObjectMsgImpl(ref_msg, ref_action);
}

PL_STATICLINK_FILE(Core, Core_World_Implementation_Declarations);
