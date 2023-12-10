#include <Core/CorePCH.h>

#include <Core/Messages/SetColorMessage.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plSetColorMode, 1)
PLASMA_ENUM_CONSTANTS(plSetColorMode::SetRGBA, plSetColorMode::SetRGB, plSetColorMode::SetAlpha, plSetColorMode::AlphaBlend, plSetColorMode::Additive, plSetColorMode::Modulate)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgSetColor);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgSetColor, 1, plRTTIDefaultAllocator<plMsgSetColor>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Color", m_Color),
    PLASMA_ENUM_MEMBER_PROPERTY("Mode", plSetColorMode, m_Mode)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plMsgSetColor::ModifyColor(plColor& color) const
{
  switch (m_Mode)
  {
    case plSetColorMode::SetRGB:
      color.SetRGB(m_Color.r, m_Color.g, m_Color.b);
      break;

    case plSetColorMode::SetAlpha:
      color.a = m_Color.a;
      break;

    case plSetColorMode::AlphaBlend:
      color = plMath::Lerp(color, m_Color, m_Color.a);
      break;

    case plSetColorMode::Additive:
      color += m_Color;
      break;

    case plSetColorMode::Modulate:
      color *= m_Color;
      break;

    case plSetColorMode::SetRGBA:
    default:
      color = m_Color;
      break;
  }
}

void plMsgSetColor::ModifyColor(plColorGammaUB& color) const
{
  plColor temp = color;
  ModifyColor(temp);
  color = temp;
}

void plMsgSetColor::Serialize(plStreamWriter& stream) const
{
  stream << m_Color;
  stream << m_Mode;
}

void plMsgSetColor::Deserialize(plStreamReader& stream, plUInt8 uiTypeVersion)
{
  stream >> m_Color;
  stream >> m_Mode;
}



PLASMA_STATICLINK_FILE(Core, Core_Messages_Implementation_SetColorMessage);
