#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <SampleGamePlugin/Components/DisplayMsgComponent.h>
#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(DisplayMsgComponent, 1, plComponentMode::Static /* this component does not move the owner node */)
{
  //PLASMA_BEGIN_PROPERTIES
  //{
  //}
  //PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("SampleGamePlugin"),
  }
  PLASMA_END_ATTRIBUTES;

  // BEGIN-DOCS-CODE-SNIPPET: message-handler-block
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgSetText, OnSetText),
    PLASMA_MESSAGE_HANDLER(plMsgSetColor, OnSetColor)
  }
  PLASMA_END_MESSAGEHANDLERS;
  // END-DOCS-CODE-SNIPPET
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

DisplayMsgComponent::DisplayMsgComponent() = default;
DisplayMsgComponent::~DisplayMsgComponent() = default;

void DisplayMsgComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
}

void DisplayMsgComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
}

void DisplayMsgComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();
}

void DisplayMsgComponent::Update()
{
  const plTransform ownerTransform = GetOwner()->GetGlobalTransform();

  plDebugRenderer::Draw3DText(GetWorld(), m_sCurrentText.GetData(), ownerTransform.m_vPosition, m_TextColor, 32);
}

// BEGIN-DOCS-CODE-SNIPPET: message-handler-impl
void DisplayMsgComponent::OnSetText(plMsgSetText& msg)
{
  m_sCurrentText = msg.m_sText;
}

void DisplayMsgComponent::OnSetColor(plMsgSetColor& msg)
{
  m_TextColor = msg.m_Color;
}
// END-DOCS-CODE-SNIPPET
