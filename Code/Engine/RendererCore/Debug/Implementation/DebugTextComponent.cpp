#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugTextComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plDebugTextComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new plDefaultValueAttribute("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")),
    PL_MEMBER_PROPERTY("Value0", m_fValue0),
    PL_MEMBER_PROPERTY("Value1", m_fValue1),
    PL_MEMBER_PROPERTY("Value2", m_fValue2),
    PL_MEMBER_PROPERTY("Value3", m_fValue3),
    PL_MEMBER_PROPERTY("Color", m_Color),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Utilities/Debug"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDebugTextComponent::plDebugTextComponent()
  : m_sText("Value0: {0}, Value1: {1}, Value2: {2}, Value3: {3}")
  , m_Color(plColor::White)
{
}

plDebugTextComponent::~plDebugTextComponent() = default;

void plDebugTextComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sText;
  s << m_fValue0;
  s << m_fValue1;
  s << m_fValue2;
  s << m_fValue3;
  s << m_Color;
}

void plDebugTextComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_sText;
  s >> m_fValue0;
  s >> m_fValue1;
  s >> m_fValue2;
  s >> m_fValue3;
  s >> m_Color;
}

void plDebugTextComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  if (!m_sText.IsEmpty())
  {
    plStringBuilder sb;
    sb.SetFormat(m_sText, m_fValue0, m_fValue1, m_fValue2, m_fValue3);

    plDebugRenderer::Draw3DText(msg.m_pView->GetHandle(), sb, GetOwner()->GetGlobalPosition(), m_Color);
  }
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Debug_Implementation_DebugTextComponent);
