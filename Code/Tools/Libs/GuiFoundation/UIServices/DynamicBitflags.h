#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// \brief Stores the valid values and names for 'dynamic' bitflags.
///
/// The names and valid values for dynamic bitflags may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicBitflags() to create or get the plDynamicBitflags for a specific type.
class PLASMA_GUIFOUNDATION_DLL plDynamicBitflags
{
public:
  /// \brief Returns a plDynamicBitflags under the given name. Creates a new one, if the name has not been used before.
  static plDynamicBitflags& GetDynamicBitflags(plStringView sName);

  /// \brief Returns all bitflag values and current names.
  const plMap<plUInt64, plString>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets stored values.
  void Clear();

  /// \brief Sets the name for the given bit position.
  void SetValueAndName(plUInt32 uiBitPos, plStringView sName);

  /// \brief Removes a value, if it exists.
  void RemoveValue(plUInt32 uiBitPos);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(plUInt32 uiBitPos) const;

  /// \brief Returns the name for the given value
  bool TryGetValueName(plUInt32 uiBitPos, plStringView& out_sName) const;

private:
  plMap<plUInt64, plString> m_ValidValues;

  static plMap<plString, plDynamicBitflags> s_DynamicBitflags;
};
