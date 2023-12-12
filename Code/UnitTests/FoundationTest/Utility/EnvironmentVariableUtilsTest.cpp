#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/System/EnvironmentVariableUtils.h>

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_UWP)

static plUInt32 uiVersionForVariableSetting = 0;

PLASMA_CREATE_SIMPLE_TEST(Utility, EnvironmentVariableUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetValueString / GetValueInt")
  {
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)

    // Windows will have "NUMBER_OF_PROCESSORS" and "USERNAME" set, let's see if we can get them
    PLASMA_TEST_BOOL(plEnvironmentVariableUtils::IsVariableSet("NUMBER_OF_PROCESSORS"));

    plInt32 iNumProcessors = plEnvironmentVariableUtils::GetValueInt("NUMBER_OF_PROCESSORS", -23);
    PLASMA_TEST_BOOL(iNumProcessors > 0);

    PLASMA_TEST_BOOL(plEnvironmentVariableUtils::IsVariableSet("USERNAME"));
    plString szUserName = plEnvironmentVariableUtils::GetValueString("USERNAME");
    PLASMA_TEST_BOOL(szUserName.GetElementCount() > 0);

#  elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX) || PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)

    // Mac OS & Linux will have "USER" set
    PLASMA_TEST_BOOL(plEnvironmentVariableUtils::IsVariableSet("USER"));
    plString szUserName = plEnvironmentVariableUtils::GetValueString("USER");
    PLASMA_TEST_BOOL(szUserName.GetElementCount() > 0);

#  endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsVariableSet/SetValue")
  {
    plStringBuilder szVarName;
    szVarName.Format("PLASMA_THIS_SHOULDNT_EXIST_NOW_OR_THIS_TEST_WILL_FAIL_{0}", uiVersionForVariableSetting++);

    PLASMA_TEST_BOOL(!plEnvironmentVariableUtils::IsVariableSet(szVarName));

    plEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE").IgnoreResult();
    PLASMA_TEST_BOOL(plEnvironmentVariableUtils::IsVariableSet(szVarName));

    PLASMA_TEST_STRING(plEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE");

    // Test overwriting the same value again
    plEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE_SOMETHING_ELSE").IgnoreResult();
    PLASMA_TEST_STRING(plEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE_SOMETHING_ELSE");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Variable with very long value")
  {
    // The Windows implementation has a 64 wchar_t buffer for example. Let's try setting a really
    // long variable and getting it back
    const char* szLongVariable =
      "SOME REALLY LONG VALUE, LETS TEST SOME LIMITS WE MIGHT HIT - 012456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";

    plStringBuilder szVarName;
    szVarName.Format("PLASMA_LONG_VARIABLE_TEST_{0}", uiVersionForVariableSetting++);

    PLASMA_TEST_BOOL(!plEnvironmentVariableUtils::IsVariableSet(szVarName));

    plEnvironmentVariableUtils::SetValueString(szVarName, szLongVariable).IgnoreResult();
    PLASMA_TEST_BOOL(plEnvironmentVariableUtils::IsVariableSet(szVarName));

    PLASMA_TEST_STRING(plEnvironmentVariableUtils::GetValueString(szVarName), szLongVariable);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Unsetting variables")
  {
    const char* szVarName = "PLASMA_TEST_HELLO_WORLD";
    PLASMA_TEST_BOOL(!plEnvironmentVariableUtils::IsVariableSet(szVarName));

    plEnvironmentVariableUtils::SetValueString(szVarName, "TEST").IgnoreResult();

    PLASMA_TEST_BOOL(plEnvironmentVariableUtils::IsVariableSet(szVarName));

    plEnvironmentVariableUtils::UnsetVariable(szVarName).IgnoreResult();
    PLASMA_TEST_BOOL(!plEnvironmentVariableUtils::IsVariableSet(szVarName));
  }
}

#endif
