#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/SendMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(SendMsgComponent, 1, plComponentMode::Static /* this component does not move the owner node */)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Strings", m_TextArray)
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("SampleGamePlugin"),
  }
  PLASMA_END_ATTRIBUTES;

  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnSendText)
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

SendMsgComponent::SendMsgComponent() = default;
SendMsgComponent::~SendMsgComponent() = default;

void SendMsgComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s.WriteArray(m_TextArray).IgnoreResult();
}

void SendMsgComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  auto& s = stream.GetStream();

  s.ReadArray(m_TextArray).IgnoreResult();
}

static plHashedString s_sSendNextString = plMakeHashedString("SendNextString");

void SendMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // start sending strings shortly
  plMsgComponentInternalTrigger msg;
  msg.m_sMessage = s_sSendNextString;
  PostMessage(msg, plTime::Milliseconds(100));
}

void SendMsgComponent::OnSendText(plMsgComponentInternalTrigger& msg)
{
  // Note: We don't need to take care to stop when the component gets deactivated
  // because messages are only delivered to active components.
  // However, if the component got deactivated and activated again within the 2 second
  // message delay, OnSimulationStarted() above could queue a second message, and now
  // both of them would arrive. We don't handle that case here.
  // if (!IsActiveAndSimulating())
  //  return;

  if (msg.m_sMessage == s_sSendNextString)
  {
    if (!m_TextArray.IsEmpty())
    {
      const plUInt32 idx = m_uiNextString % m_TextArray.GetCount();

      // send the message to all components on this node and all child nodes

      plGameObject* pGameObject = GetOwner();

      // BEGIN-DOCS-CODE-SNIPPET: message-send-direct
      plMsgSetText textMsg;
      textMsg.m_sText = m_TextArray[idx];
      pGameObject->SendMessageRecursive(textMsg);
      // END-DOCS-CODE-SNIPPET

      m_uiNextString++;
    }


    // send the next string in a second
    PostMessage(msg, plTime::Seconds(2));
  }
}
