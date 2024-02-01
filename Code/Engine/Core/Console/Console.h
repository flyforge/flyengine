#pragma once

#include <Core/Console/ConsoleFunction.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

struct PL_CORE_DLL plConsoleString
{
  enum class Type : plUInt8
  {
    Default,
    Error,
    SeriousWarning,
    Warning,
    Note,
    Success,
    Executed,
    VarName,
    FuncName,
    Dev,
    Debug,
  };

  Type m_Type = Type::Default;
  plString m_sText;
  plColor GetColor() const;

  bool operator<(const plConsoleString& rhs) const { return m_sText < rhs.m_sText; }
};

struct PL_CORE_DLL plCommandInterpreterState
{
  plStringBuilder m_sInput;
  plHybridArray<plConsoleString, 16> m_sOutput;

  void AddOutputLine(const plFormatString& text, plConsoleString::Type type = plConsoleString::Type::Default);
};

class PL_CORE_DLL plCommandInterpreter : public plRefCounted
{
public:
  virtual void Interpret(plCommandInterpreterState& inout_state) = 0;

  virtual void AutoComplete(plCommandInterpreterState& inout_state);

  /// \brief Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(plStringView sVariable, plDeque<plString>& ref_commonStrings, plDeque<plConsoleString>& ref_consoleStrings);

  /// \brief Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(plStringView sVariable, plDeque<plString>& ref_commonStrings, plDeque<plConsoleString>& ref_consoleStrings);

  /// \brief Returns the prefix string that is common to all strings in the \a vStrings array.
  static const plString FindCommonString(const plDeque<plString>& strings);
};

/// \brief The event data that is broadcast by the console
struct plConsoleEvent
{
  enum class Type : plInt32
  {
    OutputLineAdded, ///< A string was added to the console
  };

  Type m_Type;

  /// \brief The console string that was just added.
  const plConsoleString* m_AddedpConsoleString;
};

class PL_CORE_DLL plConsole
{
public:
  plConsole();
  virtual ~plConsole();

  /// \name Events
  /// @{

public:
  /// \brief Grants access to subscribe and unsubscribe from console events.
  const plEvent<const plConsoleEvent&>& Events() const { return m_Events; }

protected:
  /// \brief The console event variable, to attach to.
  plEvent<const plConsoleEvent&> m_Events;

  /// @}

  /// \name Helpers
  /// @{

public:
  /// \brief Returns the mutex that's used to prevent multi-threaded access
  plMutex& GetMutex() const { return m_Mutex; }

  static void SetMainConsole(plConsole* pConsole);
  static plConsole* GetMainConsole();

protected:
  mutable plMutex m_Mutex;

private:
  static plConsole* s_pMainConsole;

  /// @}

  /// \name Command Interpreter
  /// @{

public:
  /// \brief Replaces the current command interpreter.
  ///
  /// This base class doesn't set any default interpreter, but derived classes may do so.
  void SetCommandInterpreter(const plSharedPtr<plCommandInterpreter>& pInterpreter) { m_pCommandInterpreter = pInterpreter; }

  /// \brief Returns the currently used command interpreter.
  const plSharedPtr<plCommandInterpreter>& GetCommandInterpreter() const { return m_pCommandInterpreter; }

  /// \brief Auto-completes the given text.
  ///
  /// Returns true, if the string was modified in any way.
  /// Adds additional strings to the console output, if there are further auto-completion suggestions.
  virtual bool AutoComplete(plStringBuilder& ref_sText);

  /// \brief Executes the given input string.
  ///
  /// The command is forwarded to the set command interpreter.
  virtual void ExecuteCommand(plStringView sInput);

protected:
  plSharedPtr<plCommandInterpreter> m_pCommandInterpreter;

  /// @}

  /// \name Console Display
  /// @{

public:
  /// \brief Adds a string to the console.
  ///
  /// The base class only broadcasts an event, but does not store the string anywhere.
  virtual void AddConsoleString(plStringView sText, plConsoleString::Type type = plConsoleString::Type::Default);

  /// @}

  /// \name Input History
  /// @{

public:
  /// \brief Adds an item to the input history.
  void AddToInputHistory(plStringView sText);

  /// \brief Returns the current input history.
  ///
  /// Make sure to lock the console's mutex while working with the history.
  const plStaticArray<plString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// \brief Replaces the input line by the next (or previous) history item.
  void RetrieveInputHistory(plInt32 iHistoryUp, plStringBuilder& ref_sResult);

  /// \brief Writes the current input history to a text file.
  plResult SaveInputHistory(plStringView sFile);

  /// \brief Reads the text file and appends all lines to the input history.
  void LoadInputHistory(plStringView sFile);

protected:
  plInt32 m_iCurrentInputHistoryElement = -1;
  plStaticArray<plString, 16> m_InputHistory;

  /// @}
};
