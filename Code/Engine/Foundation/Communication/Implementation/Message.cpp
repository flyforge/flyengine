#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Message.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMessage, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

plMessageId plMessage::s_NextMsgId = 0;


void plMessage::PackageForTransfer(const plMessage& msg, plStreamWriter& inout_stream)
{
  const plRTTI* pRtti = msg.GetDynamicRTTI();

  inout_stream << pRtti->GetTypeNameHash();
  inout_stream << (plUInt8)pRtti->GetTypeVersion();

  msg.Serialize(inout_stream);
}

plUniquePtr<plMessage> plMessage::ReplicatePackedMessage(plStreamReader& inout_stream)
{
  plUInt64 uiTypeHash = 0;
  inout_stream >> uiTypeHash;

  plUInt8 uiTypeVersion = 0;
  inout_stream >> uiTypeVersion;

  const plRTTI* pRtti = plRTTI::FindTypeByNameHash(uiTypeHash);
  if (pRtti == nullptr || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  auto pMsg = pRtti->GetAllocator()->Allocate<plMessage>();

  pMsg->Deserialize(inout_stream, uiTypeVersion);

  return pMsg;
}

PL_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Message);
