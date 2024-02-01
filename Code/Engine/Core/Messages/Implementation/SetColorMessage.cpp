#include <Core/CorePCH.h>

#include <Core/Messages/SetColorMessage.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plSetColorMode, 1)
PL_ENUM_CONSTANTS(plSetColorMode::SetRGBA, plSetColorMode::SetRGB, plSetColorMode::SetAlpha, plSetColorMode::AlphaBlend, plSetColorMode::Additive, plSetColorMode::Modulate)
PL_END_STATIC_REFLECTED_ENUM;

PL_IMPLEMENT_MESSAGE_TYPE(plMsgSetColor);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetColor, 1, plRTTIDefaultAllocator<plMsgSetColor>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Color", m_Color),
    PL_ENUM_MEMBER_PROPERTY("Mode", plSetColorMode, m_Mode)
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plMsgSetColor::ModifyColor(plColor& ref_color) const
{
  switch (m_Mode)
  {
    case plSetColorMode::SetRGB:
      ref_color.SetRGB(m_Color.r, m_Color.g, m_Color.b);
      break;

    case plSetColorMode::SetAlpha:
      ref_color.a = m_Color.a;
      break;

    case plSetColorMode::AlphaBlend:
      ref_color = plMath::Lerp(ref_color, m_Color, m_Color.a);
      break;

    case plSetColorMode::Additive:
      ref_color += m_Color;
      break;

    case plSetColorMode::Modulate:
      ref_color *= m_Color;
      break;

    case plSetColorMode::SetRGBA:
    default:
      ref_color = m_Color;
      break;
  }
}

void plMsgSetColor::ModifyColor(plColorGammaUB& ref_color) const
{
  plColor temp = ref_color;
  ModifyColor(temp);
  ref_color = temp;
}

void plMsgSetColor::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream << m_Color;
  inout_stream << m_Mode;
}

void plMsgSetColor::Deserialize(plStreamReader& inout_stream, plUInt8 uiTypeVersion)
{
  inout_stream >> m_Color;
  inout_stream >> m_Mode;
}



PL_STATICLINK_FILE(Core, Core_Messages_Implementation_SetColorMessage);
