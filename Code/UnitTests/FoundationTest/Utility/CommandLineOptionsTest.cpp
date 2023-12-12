#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineOptions.h>

namespace
{
  class LogTestLogInterface : public plLogInterface
  {
  public:
    virtual void HandleLogMessage(const plLoggingEventData& le) override
    {
      switch (le.m_EventType)
      {
        case plLogMsgType::Flush:
          m_Result.Append("[Flush]\n");
          return;
        case plLogMsgType::BeginGroup:
          m_Result.Append(">", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::EndGroup:
          m_Result.Append("<", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::ErrorMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::SeriousWarningMsg:
          m_Result.Append("SW:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::WarningMsg:
          m_Result.Append("W:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::SuccessMsg:
          m_Result.Append("S:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::InfoMsg:
          m_Result.Append("I:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::DevMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case plLogMsgType::DebugMsg:
          m_Result.Append("D:", le.m_sTag, " ", le.m_sText, "\n");
          break;

        default:
          PLASMA_REPORT_FAILURE("Invalid msg type");
          break;
      }
    }

    plStringBuilder m_Result;
  };

} // namespace

PLASMA_CREATE_SIMPLE_TEST(Utility, CommandLineOptions)
{
  plCommandLineOptionDoc optDoc("__test", "-argDoc", "<doc>", "Doc argument", "no value");

  plCommandLineOptionBool optBool1("__test", "-bool1", "bool argument 1", false);
  plCommandLineOptionBool optBool2("__test", "-bool2", "bool argument 2", true);

  plCommandLineOptionInt optInt1("__test", "-int1", "int argument 1", 1);
  plCommandLineOptionInt optInt2("__test", "-int2", "int argument 2", 0, 4, 8);
  plCommandLineOptionInt optInt3("__test", "-int3", "int argument 3", 6, -8, 8);

  plCommandLineOptionFloat optFloat1("__test", "-float1", "float argument 1", 1);
  plCommandLineOptionFloat optFloat2("__test", "-float2", "float argument 2", 0, 4, 8);
  plCommandLineOptionFloat optFloat3("__test", "-float3", "float argument 3", 6, -8, 8);

  plCommandLineOptionString optString1("__test", "-string1", "string argument 1", "default string");

  plCommandLineOptionPath optPath1("__test", "-path1", "path argument 1", "default path");

  plCommandLineOptionEnum optEnum1("__test", "-enum1", "enum argument 1", "A | B = 2 | C | D | E = 7", 3);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plCommandLineOptionBool")
  {
    plCommandLineUtils cmd;
    cmd.InjectCustomArgument("-bool1");
    cmd.InjectCustomArgument("on");

    PLASMA_TEST_BOOL(optBool1.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd) == true);
    PLASMA_TEST_BOOL(optBool2.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd) == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plCommandLineOptionInt")
  {
    plCommandLineUtils cmd;
    cmd.InjectCustomArgument("-int1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-int2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-int3");
    cmd.InjectCustomArgument("-2");

    PLASMA_TEST_INT(optInt1.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), 3);
    PLASMA_TEST_INT(optInt2.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), 0);
    PLASMA_TEST_INT(optInt3.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), -2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plCommandLineOptionFloat")
  {
    plCommandLineUtils cmd;
    cmd.InjectCustomArgument("-float1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-float2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-float3");
    cmd.InjectCustomArgument("-2");

    PLASMA_TEST_FLOAT(optFloat1.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), 3, 0.001f);
    PLASMA_TEST_FLOAT(optFloat2.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), 0, 0.001f);
    PLASMA_TEST_FLOAT(optFloat3.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), -2, 0.001f);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plCommandLineOptionString")
  {
    plCommandLineUtils cmd;
    cmd.InjectCustomArgument("-string1");
    cmd.InjectCustomArgument("hello");

    PLASMA_TEST_STRING(optString1.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), "hello");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plCommandLineOptionPath")
  {
    plCommandLineUtils cmd;
    cmd.InjectCustomArgument("-path1");
    cmd.InjectCustomArgument("C:/test");

    const plString path = optPath1.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd);

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    PLASMA_TEST_STRING(path, "C:/test");
#else
    PLASMA_TEST_BOOL(path.EndsWith("C:/test"));
#endif
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plCommandLineOptionEnum")
  {
    {
      plCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("A");

      PLASMA_TEST_INT(optEnum1.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), 0);
    }

    {
      plCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("B");

      PLASMA_TEST_INT(optEnum1.GetOptionValue(plCommandLineOption::LogMode::Never, &cmd), 2);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "LogAvailableOptions")
  {
    plCommandLineUtils cmd;

    plStringBuilder result;

    PLASMA_TEST_BOOL(plCommandLineOption::LogAvailableOptionsToBuffer(result, plCommandLineOption::LogAvailableModes::Always, "__test", &cmd));

    PLASMA_TEST_STRING(result, "\
\n\
-argDoc <doc> = no value\n\
    Doc argument\n\
\n\
-bool1 <bool> = false\n\
    bool argument 1\n\
\n\
-bool2 <bool> = true\n\
    bool argument 2\n\
\n\
-int1 <int> = 1\n\
    int argument 1\n\
\n\
-int2 <int> [4 .. 8] = 0\n\
    int argument 2\n\
\n\
-int3 <int> [-8 .. 8] = 6\n\
    int argument 3\n\
\n\
-float1 <float> = 1\n\
    float argument 1\n\
\n\
-float2 <float> [4 .. 8] = 0\n\
    float argument 2\n\
\n\
-float3 <float> [-8 .. 8] = 6\n\
    float argument 3\n\
\n\
-string1 <string> = default string\n\
    string argument 1\n\
\n\
-path1 <path> = default path\n\
    path argument 1\n\
\n\
-enum1 <A | B | C | D | E> = C\n\
    enum argument 1\n\
\n\
\n\
");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsHelpRequested")
  {
    plCommandLineUtils cmd;

    PLASMA_TEST_BOOL(!plCommandLineOption::IsHelpRequested(&cmd));

    cmd.InjectCustomArgument("-help");

    PLASMA_TEST_BOOL(plCommandLineOption::IsHelpRequested(&cmd));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RequireOptions")
  {
    plCommandLineUtils cmd;
    plString missing;

    PLASMA_TEST_BOOL(plCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Failed());
    PLASMA_TEST_STRING(missing, "-opt1");

    cmd.InjectCustomArgument("-opt1");

    PLASMA_TEST_BOOL(plCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Failed());
    PLASMA_TEST_STRING(missing, "-opt2");

    cmd.InjectCustomArgument("-opt2");

    PLASMA_TEST_BOOL(plCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Succeeded());
    PLASMA_TEST_STRING(missing, "");
  }
}
