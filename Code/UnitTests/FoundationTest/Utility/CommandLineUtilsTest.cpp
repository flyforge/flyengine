#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>

PLASMA_CREATE_SIMPLE_TEST(Utility, CommandLineUtils)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetParameterCount / GetParameter")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "true", "-Test2", "off", "-Test3", "-Test4", "on", "-Test5"};

    plCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    PLASMA_TEST_INT(CmdLn.GetParameterCount(), 9);
    PLASMA_TEST_STRING(CmdLn.GetParameter(0), "bla/blub/myprogram.exe");
    PLASMA_TEST_STRING(CmdLn.GetParameter(1), "-Test1");
    PLASMA_TEST_STRING(CmdLn.GetParameter(2), "true");
    PLASMA_TEST_STRING(CmdLn.GetParameter(3), "-Test2");
    PLASMA_TEST_STRING(CmdLn.GetParameter(4), "off");
    PLASMA_TEST_STRING(CmdLn.GetParameter(5), "-Test3");
    PLASMA_TEST_STRING(CmdLn.GetParameter(6), "-Test4");
    PLASMA_TEST_STRING(CmdLn.GetParameter(7), "on");
    PLASMA_TEST_STRING(CmdLn.GetParameter(8), "-Test5");
    CmdLn.InjectCustomArgument("-duh");
    PLASMA_TEST_INT(CmdLn.GetParameterCount(), 10);
    PLASMA_TEST_STRING(CmdLn.GetParameter(9), "-duh");
    CmdLn.InjectCustomArgument("I need my Space");
    PLASMA_TEST_INT(CmdLn.GetParameterCount(), 11);
    PLASMA_TEST_STRING(CmdLn.GetParameter(10), "I need my Space");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetOptionIndex / GetStringOptionArguments  / GetStringOption")
  {
    const int argc = 15;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-opt1", "true", "false", "-opt2", "\"test2\"", "-opt3", "-opt4", "one", "two = three",
      "four", "   five  ", " six ", "-opt5", "-opt6"};

    plCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    PLASMA_TEST_INT(CmdLn.GetOptionIndex("-opt1"), 1);
    PLASMA_TEST_INT(CmdLn.GetOptionIndex("-opt2"), 4);
    PLASMA_TEST_INT(CmdLn.GetOptionIndex("-opt3"), 6);
    PLASMA_TEST_INT(CmdLn.GetOptionIndex("-opt4"), 7);
    PLASMA_TEST_INT(CmdLn.GetOptionIndex("-opt5"), 13);
    PLASMA_TEST_INT(CmdLn.GetOptionIndex("-opt6"), 14);

    PLASMA_TEST_INT(CmdLn.GetStringOptionArguments("-opt1"), 2);
    PLASMA_TEST_INT(CmdLn.GetStringOptionArguments("-opt2"), 1);
    PLASMA_TEST_INT(CmdLn.GetStringOptionArguments("-opt3"), 0);
    PLASMA_TEST_INT(CmdLn.GetStringOptionArguments("-opt4"), 5);
    PLASMA_TEST_INT(CmdLn.GetStringOptionArguments("-opt5"), 0);
    PLASMA_TEST_INT(CmdLn.GetStringOptionArguments("-opt6"), 0);

    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt1", 0), "true");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt1", 1), "false");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt1", 2, "end"), "end");

    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt2", 0), "\"test2\"");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt2", 1, "end"), "end");

    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt3", 0, "end"), "end");

    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt4", 0), "one");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt4", 1), "two = three");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt4", 2), "four");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt4", 3), "   five  ");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt4", 4), " six ");
    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt4", 5, "end"), "end");

    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt5", 0), "");

    PLASMA_TEST_STRING(CmdLn.GetStringOption("-opt6", 0), "");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetBoolOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "true", "-Test2", "off", "-Test3", "-Test4", "on", "-Test5"};

    plCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test1", true, true) == true);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test1", false, true) == false);

    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test2", true, true) == true);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test2", false, true) == false);

    // case insensitive and wrong
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test1", true) == true);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test1", false) == true);

    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test2", true) == false);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-test2", false) == false);

    // case sensitive and correct
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test1", true) == true);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test1", false) == true);

    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test2", true) == false);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test2", false) == false);

    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test3", true) == true);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test3", false) == true);

    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test4", true) == true);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test4", false) == true);

    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test5", true) == true);
    PLASMA_TEST_BOOL(CmdLn.GetBoolOption("-Test5", false) == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetIntOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23", "-Test2", "42", "-Test3", "-Test4", "-11", "-Test5"};

    plCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    PLASMA_TEST_INT(CmdLn.GetIntOption("-test1", 2, true), 2);
    PLASMA_TEST_INT(CmdLn.GetIntOption("-test2", 17, true), 17);

    // case insensitive and wrong
    PLASMA_TEST_INT(CmdLn.GetIntOption("-test1", 2), 23);
    PLASMA_TEST_INT(CmdLn.GetIntOption("-test2", 17), 42);

    // case sensitive and correct
    PLASMA_TEST_INT(CmdLn.GetIntOption("-Test1", 2), 23);
    PLASMA_TEST_INT(CmdLn.GetIntOption("-Test2", 3), 42);
    PLASMA_TEST_INT(CmdLn.GetIntOption("-Test3", 4), 4);
    PLASMA_TEST_INT(CmdLn.GetIntOption("-Test4", 5), -11);
    PLASMA_TEST_INT(CmdLn.GetIntOption("-Test5"), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetUIntOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23", "-Test2", "42", "-Test3", "-Test4", "-11", "-Test5"};

    plCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-test1", 2, true), 2);
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-test2", 17, true), 17);

    // case insensitive and wrong
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-test1", 2), 23);
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-test2", 17), 42);

    // case sensitive and correct
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-Test1", 2), 23);
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-Test2", 3), 42);
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-Test3", 4), 4);
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-Test4", 5), 5);
    PLASMA_TEST_INT(CmdLn.GetUIntOption("-Test5"), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFloatOption")
  {
    const int argc = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23.45", "-Test2", "42.3", "-Test3", "-Test4", "-11", "-Test5"};

    plCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3, true), 2.3, 0.0);
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8, true), 17.8, 0.0);

    // case insensitive and wrong
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3), 23.45, 0.0);
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8), 42.3, 0.0);

    // case sensitive and correct
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-Test1", 2.3), 23.45, 0.0);
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-Test2", 3.4), 42.3, 0.0);
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-Test3", 4.5), 4.5, 0.0);
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-Test4", 5.6), -11, 0.0);
    PLASMA_TEST_DOUBLE(CmdLn.GetFloatOption("-Test5"), 0, 0.0);
  }
}