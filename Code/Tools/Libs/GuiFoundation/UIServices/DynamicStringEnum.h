#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// \brief Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the plDynamicEnum for a specific type.
class PL_GUIFOUNDATION_DLL plDynamicStringEnum
{
public:
  /// \brief Returns a plDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  ///
  /// Calls s_RequestUnknownCallback, if the requested enum is not known yet, which will try to load the data.
  static plDynamicStringEnum& GetDynamicEnum(plStringView sEnumName);

  static plDynamicStringEnum& CreateDynamicEnum(plStringView sEnumName);

  /// \brief Removes the entire enum with the given name.
  static void RemoveEnum(plStringView sEnumName);

  /// \brief Returns all enum values and current names.
  const plHybridArray<plString, 16>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given enum value.
  void AddValidValue(plStringView sValue, bool bSortValues = false);

  /// \brief Removes a certain enum value, if it exists.
  void RemoveValue(plStringView sValue);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(plStringView sValue) const;

  /// \brief Sorts existing values alphabetically
  void SortValues();

  /// \brief If set to non-empty, the user can easily edit this enum through a simple dialog and the values will be saved in this file.
  ///
  /// Empty by default, as most dynamic enums need to be set up according to other criteria.
  void SetStorageFile(plStringView sFile) { m_sStorageFile = sFile; }

  /// \brief The file where values will be stored.
  plStringView GetStorageFile() const { return m_sStorageFile; }

  void ReadFromStorage();

  void SaveToStorage();

  /// \brief Invoked by GetDynamicEnum() for enums that are unkonwn at that time.
  ///
  /// Can be used to on-demand load those values, before GetDynamicEnum() returns.
  static plDelegate<void(plStringView sEnumName, plDynamicStringEnum& e)> s_RequestUnknownCallback;

private:
  plHybridArray<plString, 16> m_ValidValues;
  plString m_sStorageFile;

  static plMap<plString, plDynamicStringEnum> s_DynamicEnums;
};
