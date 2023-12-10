#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/EnumerableClass.h>

class plStringBuilder;
class plLogInterface;

/// \brief plCommandLineOption (and derived types) are used to define options that the application supports.
///
/// Command line options are created as global variables anywhere throughout the code, wherever they are needed.
/// The point of using them over going through plCommandLineUtils directly, is that the options can be listed automatically
/// and thus an application can print all available options, when the user requests help.
///
/// Consequently, their main purpose is to make options discoverable and to document them in a consistent manner.
///
/// Additionally, classes like plCommandLineOptionEnum add functionality that makes some options easier to setup.
class PLASMA_FOUNDATION_DLL plCommandLineOption : public plEnumerable<plCommandLineOption>
{
  PLASMA_DECLARE_ENUMERABLE_CLASS(plCommandLineOption);

public:
  enum class LogAvailableModes
  {
    Always,         ///< Logs the available modes no matter what
    IfHelpRequested ///< Only logs the modes, if '-h', '-help', '-?' or something similar was specified
  };

  /// \brief Describes whether the value of an option (and whether something went wrong), should be printed to plLog.
  enum class LogMode
  {
    Never,                ///< Don't log anything.
    FirstTime,            ///< Only print the information the first time a value is accessed.
    FirstTimeIfSpecified, ///< Only on first access and only if the user specified the value on the command line.
    Always,               ///< Always log the options value on access.
    AlwaysIfSpecified,    ///< Always log values, if the user specified non-default ones.
  };

  /// \brief Checks whether a command line was passed that requests help output.
  static bool IsHelpRequested(const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()); // [tested]

  /// \brief Checks whether all required options are passed to the command line.
  ///
  /// The options are passed as a semicolon-separated list (spare spaces are stripped away), for instance "-opt1; -opt2"
  static plResult RequireOptions(plStringView sRequiredOptions, plString* pMissingOption = nullptr, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()); // [tested]

  /// \brief Prints all available options to the plLog.
  ///
  /// \param szGroupFilter
  ///   If this is empty, all options from all 'sorting groups' are logged.
  ///   If non-empty, only options from sorting groups that appear in this string will be logged.
  static bool LogAvailableOptions(LogAvailableModes mode, plStringView sGroupFilter = {}, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()); // [tested]

  /// \brief Same as LogAvailableOptions() but captures the output from plLog and returns it in an plStringBuilder.
  static bool LogAvailableOptionsToBuffer(plStringBuilder& out_sBuffer, LogAvailableModes mode, plStringView sGroupFilter = {}, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()); // [tested]

public:
  /// \param szSortingGroup
  ///   This string is used to sort options. Application options should start with an underscore, such that they appear first
  ///   in the output.
  plCommandLineOption(plStringView sSortingGroup) { m_sSortingGroup = sSortingGroup; }

  /// \brief Writes the sorting group name to 'out'.
  virtual void GetSortingGroup(plStringBuilder& ref_sOut) const;

  /// \brief Writes all the supported options (e.g. '-arg') to 'out'.
  /// If more than one option is allowed, they should be separated with semicolons or pipes.
  virtual void GetOptions(plStringBuilder& ref_sOut) const = 0;

  /// \brief Returns the supported option names (e.g. '-arg') as split strings.
  void GetSplitOptions(plStringBuilder& out_sAll, plDynamicArray<plStringView>& ref_splitOptions) const;

  /// \brief Returns a very short description of the option (type). For example "<int>" or "<enum>".
  virtual void GetParamShortDesc(plStringBuilder& ref_sOut) const = 0;

  /// \brief Returns a very short string for the options default value. For example "0" or "auto".
  virtual void GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const = 0;

  /// \brief Returns a proper description of the option.
  ///
  /// The long description is allowed to contain newlines (\n) and the output will be formatted accordingly.
  virtual void GetLongDesc(plStringBuilder& ref_sOut) const = 0;

  /// \brief Returns a string indicating the exact implementation type.
  virtual plStringView GetType() = 0;

protected:
  plStringView m_sSortingGroup;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief plCommandLineOptionDoc can be used to document a command line option whose logic might be more complex than what the other option types provide.
///
/// This class is meant to be used for options that are actually queried directly through plCommandLineUtils,
/// but should still show up in the command line option documentation, such that the user can discover them.
///
class PLASMA_FOUNDATION_DLL plCommandLineOptionDoc : public plCommandLineOption
{
public:
  plCommandLineOptionDoc(plStringView sSortingGroup, plStringView sArgument, plStringView sParamShortDesc, plStringView sLongDesc, plStringView sDefaultValue, bool bCaseSensitive = false);

  virtual void GetOptions(plStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(plStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetLongDesc(plStringBuilder& ref_sOut) const override; // [tested]

  /// \brief Returns "Doc"
  virtual plStringView GetType() override { return "Doc"; }

  /// \brief Checks whether any of the option variants is set on the command line, and returns which one. For example '-h' or '-help'.
  bool IsOptionSpecified(plStringBuilder* out_pWhich = nullptr, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()) const; // [tested]

protected:
  bool ShouldLog(LogMode mode, bool bWasSpecified) const;
  void LogOption(plStringView sOption, plStringView sValue, bool bWasSpecified) const;

  plStringView m_sArgument;
  plStringView m_sParamShortDesc;
  plStringView m_sParamDefaultValue;
  plStringView m_sLongDesc;
  bool m_bCaseSensitive = false;
  mutable bool m_bLoggedOnce = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief This command line option exposes simple on/off switches.
class PLASMA_FOUNDATION_DLL plCommandLineOptionBool : public plCommandLineOptionDoc
{
public:
  plCommandLineOptionBool(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, bool bDefaultValue, bool bCaseSensitive = false);

  /// \brief Returns the value of this option. Either what was specified on the command line, or the default value.
  bool GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// \brief Modifies the default value
  void SetDefaultValue(bool value)
  {
    m_bDefaultValue = value;
  }

  /// \brief Returns the default value.
  bool GetDefaultValue() const { return m_bDefaultValue; }

  /// \brief Returns "Bool"
  virtual plStringView GetType() override { return "Bool"; }

protected:
  bool m_bDefaultValue = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief This command line option exposes integer values, optionally with a min/max range.
///
/// If the user specified a value outside the allowed range, a warning is printed, and the default value is used instead.
/// It is valid for the default value to be outside the min/max range, which can be used to detect whether the user provided any value at all.
class PLASMA_FOUNDATION_DLL plCommandLineOptionInt : public plCommandLineOptionDoc
{
public:
  plCommandLineOptionInt(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, int iDefaultValue, int iMinValue = plMath::MinValue<int>(), int iMaxValue = plMath::MaxValue<int>(), bool bCaseSensitive = false);

  virtual void GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(plStringBuilder& ref_sOut) const override; // [tested]

  /// \brief Returns the value of this option. Either what was specified on the command line, or the default value.
  int GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// \brief Modifies the default value
  void SetDefaultValue(plInt32 value)
  {
    m_iDefaultValue = value;
  }

  /// \brief Returns "Int"
  virtual plStringView GetType() override { return "Int"; }

  /// \brief Returns the minimum value.
  plInt32 GetMinValue() const { return m_iMinValue; }

  /// \brief Returns the maximum value.
  plInt32 GetMaxValue() const { return m_iMaxValue; }

  /// \brief Returns the default value.
  plInt32 GetDefaultValue() const { return m_iDefaultValue; }

protected:
  plInt32 m_iDefaultValue = 0;
  plInt32 m_iMinValue = 0;
  plInt32 m_iMaxValue = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief This command line option exposes float values, optionally with a min/max range.
///
/// If the user specified a value outside the allowed range, a warning is printed, and the default value is used instead.
/// It is valid for the default value to be outside the min/max range, which can be used to detect whether the user provided any value at all.
class PLASMA_FOUNDATION_DLL plCommandLineOptionFloat : public plCommandLineOptionDoc
{
public:
  plCommandLineOptionFloat(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, float fDefaultValue, float fMinValue = plMath::MinValue<float>(), float fMaxValue = plMath::MaxValue<float>(), bool bCaseSensitive = false);

  virtual void GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamShortDesc(plStringBuilder& ref_sOut) const override; // [tested]

  /// \brief Returns the value of this option. Either what was specified on the command line, or the default value.
  float GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// \brief Modifies the default value
  void SetDefaultValue(float value)
  {
    m_fDefaultValue = value;
  }

  /// \brief Returns "Float"
  virtual plStringView GetType() override { return "Float"; }

  /// \brief Returns the minimum value.
  float GetMinValue() const { return m_fMinValue; }

  /// \brief Returns the maximum value.
  float GetMaxValue() const { return m_fMaxValue; }

  /// \brief Returns the default value.
  float GetDefaultValue() const { return m_fDefaultValue; }

protected:
  float m_fDefaultValue = 0;
  float m_fMinValue = 0;
  float m_fMaxValue = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief This command line option exposes simple string values.
class PLASMA_FOUNDATION_DLL plCommandLineOptionString : public plCommandLineOptionDoc
{
public:
  plCommandLineOptionString(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, plStringView sDefaultValue, bool bCaseSensitive = false);

  /// \brief Returns the value of this option. Either what was specified on the command line, or the default value.
  plStringView GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// \brief Modifies the default value
  void SetDefaultValue(plStringView sValue)
  {
    m_sDefaultValue = sValue;
  }

  /// \brief Returns the default value.
  plStringView GetDefaultValue() const { return m_sDefaultValue; }

  /// \brief Returns "String"
  virtual plStringView GetType() override { return "String"; }

protected:
  plStringView m_sDefaultValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief This command line option exposes absolute paths. If the user provides a relative path, it will be concatenated with the current working directory.
class PLASMA_FOUNDATION_DLL plCommandLineOptionPath : public plCommandLineOptionDoc
{
public:
  plCommandLineOptionPath(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, plStringView sDefaultValue, bool bCaseSensitive = false);

  /// \brief Returns the value of this option. Either what was specified on the command line, or the default value.
  plString GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()) const; // [tested]

  /// \brief Modifies the default value
  void SetDefaultValue(plStringView sValue)
  {
    m_sDefaultValue = sValue;
  }

  /// \brief Returns the default value.
  plStringView GetDefaultValue() const { return m_sDefaultValue; }

  /// \brief Returns "Path"
  virtual plStringView GetType() override { return "Path"; }

protected:
  plStringView m_sDefaultValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief An 'enum' option is a string option that only allows certain phrases ('keys').
///
/// Each phrase has an integer value, and GetOptionValue() returns the integer value of the selected phrase.
/// It is valid for the default value to be different from all the phrase values,
/// which can be used to detect whether the user provided any phrase at all.
///
/// The allowed values are passed in as a single string, in the form "OptA = 0 | OptB = 1 | ..."
/// Phrase values ("= 0" etc) are optional, and if not given are automatically assigned starting at zero.
/// Multiple phrases may share the same value.
class PLASMA_FOUNDATION_DLL plCommandLineOptionEnum : public plCommandLineOptionDoc
{
public:
  plCommandLineOptionEnum(plStringView sSortingGroup, plStringView sArgument, plStringView sLongDesc, plStringView sEnumKeysAndValues, plInt32 iDefaultValue, bool bCaseSensitive = false);

  /// \brief Returns the value of this option. Either what was specified on the command line, or the default value.
  plInt32 GetOptionValue(LogMode logMode, const plCommandLineUtils* pUtils = plCommandLineUtils::GetGlobalInstance()) const; // [tested]

  virtual void GetParamShortDesc(plStringBuilder& ref_sOut) const override; // [tested]

  virtual void GetParamDefaultValueDesc(plStringBuilder& ref_sOut) const override; // [tested]

  struct EnumKeyValue
  {
    plStringView m_Key;
    plInt32 m_iValue = 0;
  };

  /// \brief Returns the enum keys (names) and values (integers) extracted from the string that was passed to the constructor.
  void GetEnumKeysAndValues(plDynamicArray<EnumKeyValue>& out_keysAndValues) const;

  /// \brief Modifies the default value
  void SetDefaultValue(plInt32 value)
  {
    m_iDefaultValue = value;
  }

  /// \brief Returns the default value.
  plInt32 GetDefaultValue() const { return m_iDefaultValue; }

  /// \brief Returns "Enum"
  virtual plStringView GetType() override { return "Enum"; }

protected:
  plInt32 m_iDefaultValue = 0;
  plStringView m_sEnumKeysAndValues;
};
