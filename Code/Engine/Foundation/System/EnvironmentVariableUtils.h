#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

/// \brief This is a helper class to interact with environment variables.
class PLASMA_FOUNDATION_DLL plEnvironmentVariableUtils
{
public:
  /// \brief Returns the current value of the request environment variable. If it isn't set szDefault will be returned.
  static plString GetValueString(plStringView sName, plStringView sDefault = nullptr);

  /// \brief Sets the environment variable for the current execution environment (i.e. this process and child processes created after this call).
  static plResult SetValueString(plStringView sName, plStringView sValue);

  /// \brief Returns the current value of the request environment variable. If it isn't set iDefault will be returned.
  static plInt32 GetValueInt(plStringView sName, plInt32 iDefault = -1);

  /// \brief Sets the environment variable for the current execution environment.
  static plResult SetValueInt(plStringView sName, plInt32 iValue);

  /// \brief Returns true if the environment variable with the given name is set, false otherwise.
  static bool IsVariableSet(plStringView sName);

  /// \brief Removes an environment variable from the current execution context (i.e. this process and child processes created after this call).
  static plResult UnsetVariable(plStringView sName);

private:
  /// \brief [internal]
  static plString GetValueStringImpl(plStringView sName, plStringView sDefault);

  /// \brief [internal]
  static plResult SetValueStringImpl(plStringView sName, plStringView sValue);

  /// \brief [internal]
  static bool IsVariableSetImpl(plStringView sName);

  /// \brief [internal]
  static plResult UnsetVariableImpl(plStringView sName);
};
