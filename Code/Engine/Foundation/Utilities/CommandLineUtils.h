#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

/// \brief This is a helper class to parse command lines.
///
/// Initialize it using SetCommandLine(). Then query for command line options using GetStringOption(), GetBoolOption(), GetIntOption()
/// or GetFloatOption()
class PLASMA_FOUNDATION_DLL plCommandLineUtils
{
public:
  enum ArgMode
  {
    UseArgcArgv,  ///< Use the passed in argc/argv values as they are passed in
    PreferOsArgs, ///< On Windows, ignore argc/argv and instead query the global arguments from the OS. Necessary to properly support
                  ///< Unicode strings in arguments.
  };

  /// \brief Returns one global instance of plCommandLineUtils.
  static plCommandLineUtils* GetGlobalInstance();

  /// \brief Splits a string into the classic argc/argv string.
  ///
  /// Useful for platforms where command line args come in as a single string.
  /// \param addExecutableDir
  ///   Adds executable path as first parameter (just as it would normally be in 'int main(argc, argv)').
  static void SplitCommandLineString(const char* szCommandString, bool bAddExecutableDir, plDynamicArray<plString>& out_args, plDynamicArray<const char*>& out_argsV);

  /// \brief Initializes plCommandLineUtils from the parameter arguments that were passed to the application.
  void SetCommandLine(plUInt32 uiArgc, const char** pArgv, ArgMode mode = UseArgcArgv); // [tested]

  /// \brief Initializes plCommandLineUtils from a list of already split up commands.
  void SetCommandLine(plArrayPtr<plString> commands);

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  /// \brief Initializes plCommandLineUtils by querying the command line parameters directly from the OS.
  ///
  /// This function is not available on all platforms.
  void SetCommandLine();
#endif

  /// \brief Returns the split up command line.
  const plDynamicArray<plString>& GetCommandLineArray() const;

  /// \brief Assembles the original command line from the split up string representation.
  plString GetCommandLineString() const;

  /// \brief Returns the total number of command line parameters (excluding the program path, which is often passed as the first parameter).
  plUInt32 GetParameterCount() const; // [tested]

  /// \brief Returns the n-th parameter string that was passed to the application.
  const plString& GetParameter(plUInt32 uiParam) const; // [tested]

  /// \brief Returns the index at which the given option string can be found in the parameter list.
  ///
  /// \param szOption
  ///   The name of the command line option. Must start with a hyphen (-)
  /// \param bCaseSensitive
  ///   Whether the option name szOption shall be searched case sensitive.
  ///
  /// \return
  ///  -1 When no option with the given name is found.
  ///  Otherwise the index at which the option can be found. This can be passed to GetParameter() or GetStringOptionArguments().
  plInt32 GetOptionIndex(plStringView sOption, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns whether the requested option is specified, at all.
  bool HasOption(plStringView sOption, bool bCaseSensitive = false) const;

  /// \brief Returns how many arguments follow behind the option with the name \a szOption.
  ///
  /// Everything that does not start with a hyphen is considered to be an additional parameter for the option.
  plUInt32 GetStringOptionArguments(plStringView sOption, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns the n-th parameter to the command line option with the name \a szOption.
  ///
  /// If the option does not exist or does not have that many parameters, \a szDefault is returned.
  plStringView GetStringOption(plStringView sOption, plUInt32 uiArgument = 0, plStringView sDefault = {},
    bool bCaseSensitive = false) const; // [tested]

  /// \brief Similar to GetStringOption() but assumes that the strings represent paths and concatenates the current working directory if a relative
  /// path is given.
  ///
  /// To check how many arguments are available, use GetStringOptionArguments().
  /// \note This function always returns absolute or rooted paths, never relative ones. If relative paths are supposed to be allowed,
  /// use GetStringOption() instead.
  ///
  /// If szDefault is empty and the user did not provide this option, then the result will also be the empty string.
  /// If szDefault is a relative path, it will be concatenated with the CWD just as any user provided option would.
  const plString GetAbsolutePathOption(plStringView sOption, plUInt32 uiArgument = 0, plStringView sDefault = {}, bool bCaseSensitive = false) const;

  /// \brief Returns a boolean interpretation of the option \a szOption or bDefault if it cannot be found.
  ///
  /// \param szOption
  ///   The name of the option to search for. All option-names must start with a hyphen.
  ///
  /// \param bDefault
  ///   The default value to use when no other value can be derived.
  ///
  /// \param bCaseSensitive
  ///   Whether it should be searched case-sensitive for the option with name \a szOption.
  ///
  /// \return
  ///   If an option with the name \a szOption can be found, which has no parameters, it is interpreted as 'true'.
  ///   If there is one parameter following, it is interpreted using plConversionUtils::StringToBool().
  ///   If that conversion fails, bDefault is returned.
  bool GetBoolOption(plStringView sOption, bool bDefault = false, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns an integer interpretation of the option \a szOption or iDefault if it cannot be found.
  ///
  /// \param szOption
  ///   The name of the option to search for. All option-names must start with a hyphen.
  ///
  /// \param iDefault
  ///   The default value to use when no other value can be derived.
  ///
  /// \param bCaseSensitive
  ///   Whether it should be searched case-sensitive for the option with name \a szOption.
  ///
  /// \return
  ///   If an option with the name \a szOption can be found, and there is one parameter following,
  ///   it is interpreted using plConversionUtils::StringToInt().
  ///   If that conversion fails or there is no such option or no parameter follows it, iDefault is returned.
  plInt32 GetIntOption(plStringView sOption, plInt32 iDefault = 0, bool bCaseSensitive = false) const; // [tested]

  /// \brief Same as GetIntOption() but assumes the value is a uint32.
  plUInt32 GetUIntOption(plStringView sOption, plUInt32 uiDefault = 0, bool bCaseSensitive = false) const; // [tested]

  /// \brief Returns a float interpretation of the option \a szOption or fDefault if it cannot be found.
  ///
  /// \param szOption
  ///   The name of the option to search for. All option-names must start with a hyphen.
  ///
  /// \param fDefault
  ///   The default value to use when no other value can be derived.
  ///
  /// \param bCaseSensitive
  ///   Whether it should be searched case-sensitive for the option with name \a szOption.
  ///
  /// \return
  ///   If an option with the name \a szOption can be found, and there is one parameter following,
  ///   it is interpreted using plConversionUtils::StringToFloat().
  ///   If that conversion fails or there is no such option or no parameter follows it, fDefault is returned.
  double GetFloatOption(plStringView sOption, double fDefault = 0.0, bool bCaseSensitive = false) const; // [tested]

  /// \brief This allows to append an argument programmatically, that wasn't actually set through the command line.
  ///
  /// This can be useful when the command-line is a method to configure something, which might be hidden away in a plugin,
  /// and we have no other easy way to configure it.
  ///
  /// Be aware that each call to this function is like one command line argument. Therefore to add "-arg test", call it two times,
  /// once with "-arg", once with "test". To add a string with spaces, call it once, but do not wrap the string in artificial quotes.
  void InjectCustomArgument(plStringView sArgument); // [tested]

private:
  plDynamicArray<plString> m_Commands;
};
