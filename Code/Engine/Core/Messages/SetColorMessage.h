#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Communication/Message.h>

/// \brief Describes how a color should be applied to another color.
struct plSetColorMode
{
  using StorageType = plUInt32;

  enum Enum
  {
    SetRGBA,  ///< Overrides all four RGBA values.
    SetRGB,   ///< Overrides the RGB values but leaves Alpha untouched.
    SetAlpha, ///< Overrides Alpha, leaves RGB untouched.

    AlphaBlend, ///< Modifies the target RGBA values by interpolating from the previous color towards the incoming color using the incoming alpha value.
    Additive,   ///< Adds to the RGBA values.
    Modulate,   /// Multiplies the RGBA values.

    Default = SetRGBA
  };
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plSetColorMode);

/// \brief A message to modify the main color of some thing.
///
/// Components that handle this message use it to change their main color.
/// For instance a light component may change its light color, a mesh component will change the main mesh color.
struct PL_CORE_DLL plMsgSetColor : public plMessage
{
  PL_DECLARE_MESSAGE_TYPE(plMsgSetColor, plMessage);

  /// \brief The color to apply to the target.
  plColor m_Color;

  /// \brief The mode with which to apply the color to the target.
  plEnum<plSetColorMode> m_Mode;

  /// \brief Applies m_Color using m_Mode to the given color.
  void ModifyColor(plColor& ref_color) const;

  /// \brief Applies m_Color using m_Mode to the given color.
  void ModifyColor(plColorGammaUB& ref_color) const;

  virtual void Serialize(plStreamWriter& inout_stream) const override;
  virtual void Deserialize(plStreamReader& inout_stream, plUInt8 uiTypeVersion) override;
};
