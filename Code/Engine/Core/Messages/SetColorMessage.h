#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

struct plSetColorMode
{
  using StorageType = plUInt32;

  enum Enum
  {
    SetRGBA,
    SetRGB,
    SetAlpha,

    AlphaBlend,
    Additive,
    Modulate,

    Default = SetRGBA
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plSetColorMode);

struct PLASMA_CORE_DLL plMsgSetColor : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgSetColor, plMessage);

  plColor m_Color;
  plEnum<plSetColorMode> m_Mode;

  void ModifyColor(plColor& color) const;
  void ModifyColor(plColorGammaUB& color) const;

  //////////////////////////////////////////////////////////////////////////
  // plMessage interface
  //

  virtual void Serialize(plStreamWriter& stream) const override;
  virtual void Deserialize(plStreamReader& stream, plUInt8 uiTypeVersion) override;
};
