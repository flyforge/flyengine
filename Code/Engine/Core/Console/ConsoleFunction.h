#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Utilities/EnumerableClass.h>

/// \brief Base class for all types of plConsoleFunction, represents functions to be exposed to plConsole.
///
/// Console functions are similar to plCVar's in that they can be executed from the plConsole.
/// A console function can wrap many different types of functions with differing number and types of parameters.
/// plConsoleFunction uses an plDelegate internally to store the function reference, so even member functions would be possible.
///
/// All console functions are enumerable, as their base class plConsoleFunctionBase is an plEnumerable class.
///
/// Console functions can have between zero and six parameters. The LuaInterpreter for plConsole only supports parameter types
/// (unsigned) int, float/double, bool and string and uses the conversion feature of plVariant to map the lua input to the final function.
///
/// To make a function available as a console function, create a global variable of type plConsoleFunction with the proper template
/// arguments to mirror its parameters and return type.
/// Note that although functions with return types are accepted, the return value is currently always ignored.
///
/// \code{.cpp}
///   void MyConsoleFunc1(int a, float b, plStringView sz) { ... }
///   plConsoleFunction<void ()> ConFunc_MyConsoleFunc1("MyConsoleFunc1", "()", MyConsoleFunc1);
///
///   int MyConsoleFunc2(int a, float b, plStringView sz) { ... }
///   plConsoleFunction<int (int, float, plString)> ConFunc_MyConsoleFunc2("MyConsoleFunc2", "(int a, float b, string c)", MyConsoleFunc2);
/// \endcode
///
/// Here the global function MyConsoleFunc2 is exposed to the console. The return value type and parameter types are passed as template
/// arguments. ConFunc_MyConsoleFunc2 is now the global variable that represents the function for the console.
/// The first string is the name with which the function is exposed, which is also used for auto-completion.
/// The second string is the description of the function. Here we inserted the parameter list with types, so that the user knows how to
/// use it. Finally the last parameter is the actual function to expose.
class PL_CORE_DLL plConsoleFunctionBase : public plEnumerable<plConsoleFunctionBase>
{
  PL_DECLARE_ENUMERABLE_CLASS(plConsoleFunctionBase);

public:
  /// \brief The constructor takes the function name and description as it should appear in the console.
  plConsoleFunctionBase(plStringView sFunctionName, plStringView sDescription)
    : m_sFunctionName(sFunctionName)
    , m_sDescription(sDescription)
  {
  }

  /// \brief Returns the name of the function as it should be exposed in the console.
  plStringView GetName() const { return m_sFunctionName; }

  /// \brief Returns the description of the function as it should appear in the console.
  plStringView GetDescription() const { return m_sDescription; }

  /// \brief Returns the number of parameters that this function takes.
  virtual plUInt32 GetNumParameters() const = 0;

  /// \brief Returns the type of the n-th parameter.
  virtual plVariant::Type::Enum GetParameterType(plUInt32 uiParam) const = 0;

  /// \brief Calls the function. Each parameter must be put into an plVariant and all of them are passed along as an array.
  ///
  /// Returns PL_FAILURE, if the number of parameters did not match, or any parameter was not convertible to the actual type that
  /// the function expects.
  virtual plResult Call(plArrayPtr<plVariant> params) = 0;

private:
  plStringView m_sFunctionName;
  plStringView m_sDescription;
};


/// \brief Implements the functionality of plConsoleFunctionBase for functions with different parameter types. See plConsoleFunctionBase for more
/// details.
template <typename R>
class plConsoleFunction : public plConsoleFunctionBase
{
};


#define ARG_COUNT 0
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 1
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 2
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 3
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 4
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 5
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT

#define ARG_COUNT 6
#include <Core/Console/Implementation/ConsoleFunctionHelper_inl.h>
#undef ARG_COUNT
