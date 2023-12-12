#include <CoreTest/CoreTestPCH.h>

#include <Core/Scripting/LuaWrapper.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Scripting);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static const char* g_Script = "\
globaltable = true;\n\
function f_globaltable()\n\
end\n\
function f_NotWorking()\n\
  DoNothing();\n\
end\n\
intvar1 = 4;\n\
intvar2 = 7;\n\
floatvar1 = 4.3;\n\
floatvar2 = 7.3;\n\
boolvar1 = true;\n\
boolvar2 = false;\n\
stringvar1 = \"zweiundvierzig\";\n\
stringvar2 = \"OhWhatsInHere\";\n\
\n\
\n\
function f1()\n\
end\n\
\n\
function f2()\n\
end\n\
\n\
MyTable =\n\
{\n\
  table1 = true;\n\
  \n\
  f_table1 = function()\n\
  end;\n\
  intvar1 = 14;\n\
  intvar2 = 17;\n\
  floatvar1 = 14.3;\n\
  floatvar2 = 17.3;\n\
  boolvar1 = false;\n\
  boolvar2 = true;\n\
  stringvar1 = \"+zweiundvierzig\";\n\
  stringvar2 = \"+OhWhatsInHere\";\n\
  \n\
  SubTable =\n\
  {\n\
    table2 = true;\n\
    f_table2 = function()\n\
    end;\n\
    intvar1 = 24;\n\
  };\n\
};\n\
\n\
";

class ScriptLog : public plLogInterface
{
public:
  virtual void HandleLogMessage(const plLoggingEventData& le) override
  {
    PLASMA_TEST_FAILURE("Script Error", le.m_sText);
    PLASMA_TEST_DEBUG_BREAK;
  }
};

class ScriptLogIgnore : public plLogInterface
{
public:
  static plInt32 g_iErrors;

  virtual void HandleLogMessage(const plLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case plLogMsgType::ErrorMsg:
      case plLogMsgType::SeriousWarningMsg:
      case plLogMsgType::WarningMsg:
        ++g_iErrors;
      default:
        break;
    }
  }
};

plInt32 ScriptLogIgnore::g_iErrors = 0;

int MyFunc1(lua_State* pState)
{
  plLuaWrapper s(pState);

  PLASMA_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  return s.ReturnToScript();
}

int MyFunc2(lua_State* pState)
{
  plLuaWrapper s(pState);

  PLASMA_TEST_INT(s.GetNumberOfFunctionParameters(), 6);
  PLASMA_TEST_BOOL(s.IsParameterBool(0));
  PLASMA_TEST_BOOL(s.IsParameterFloat(1));
  PLASMA_TEST_BOOL(s.IsParameterInt(2));
  PLASMA_TEST_BOOL(s.IsParameterNil(3));
  PLASMA_TEST_BOOL(s.IsParameterString(4));
  PLASMA_TEST_BOOL(s.IsParameterString(5));

  PLASMA_TEST_BOOL(s.GetBoolParameter(0) == true);
  PLASMA_TEST_FLOAT(s.GetFloatParameter(1), 2.3f, 0.0001f);
  PLASMA_TEST_INT(s.GetIntParameter(2), 42);
  PLASMA_TEST_STRING(s.GetStringParameter(4), "test");
  PLASMA_TEST_STRING(s.GetStringParameter(5), "tut");

  return s.ReturnToScript();
}

int MyFunc3(lua_State* pState)
{
  plLuaWrapper s(pState);

  PLASMA_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  s.PushReturnValue(false);
  s.PushReturnValue(2.3f);
  s.PushReturnValue(42);
  s.PushReturnValueNil();
  s.PushReturnValue("test");
  s.PushReturnValue("tuttut", 3);

  return s.ReturnToScript();
}

int MyFunc4(lua_State* pState)
{
  plLuaWrapper s(pState);

  PLASMA_TEST_INT(s.GetNumberOfFunctionParameters(), 1);

  PLASMA_TEST_BOOL(s.IsParameterTable(0));

  PLASMA_TEST_BOOL(s.OpenTableFromParameter(0) == PLASMA_SUCCESS);

  PLASMA_TEST_BOOL(s.IsVariableAvailable("table1") == true);

  s.CloseAllTables();

  return s.ReturnToScript();
}

PLASMA_CREATE_SIMPLE_TEST(Scripting, LuaWrapper)
{
  ScriptLog Log;
  ScriptLogIgnore LogIgnore;

  plLuaWrapper sMain;
  PLASMA_TEST_BOOL(sMain.ExecuteString(g_Script, "MainScript", &Log) == PLASMA_SUCCESS);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExecuteString")
  {
    plLuaWrapper s;
    ScriptLogIgnore::g_iErrors = 0;

    PLASMA_TEST_BOOL(s.ExecuteString(" pups ", "FailToCompile", &LogIgnore) == PLASMA_FAILURE);
    PLASMA_TEST_INT(ScriptLogIgnore::g_iErrors, 1);

    PLASMA_TEST_BOOL(s.ExecuteString(" pups(); ", "FailToExecute", &LogIgnore) == PLASMA_FAILURE);
    PLASMA_TEST_INT(ScriptLogIgnore::g_iErrors, 2);

    PLASMA_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == PLASMA_SUCCESS);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    plLuaWrapper s;
    PLASMA_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(s.IsVariableAvailable("globaltable") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("boolvar1") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("boolvar2") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("intvar1") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("intvar2") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("floatvar1") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("floatvar2") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("stringvar1") == true);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("stringvar2") == true);

    s.Clear();

    // after clearing the script, these variables should not be available anymore
    PLASMA_TEST_BOOL(s.IsVariableAvailable("globaltable") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("boolvar1") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("boolvar2") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("intvar1") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("intvar2") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("floatvar1") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("floatvar2") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("stringvar1") == false);
    PLASMA_TEST_BOOL(s.IsVariableAvailable("stringvar2") == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsVariableAvailable (Global)")
  {
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("nonexisting1") == false);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("boolvar1") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("boolvar2") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("nonexisting2") == false);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("intvar1") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("intvar2") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("nonexisting3") == false);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("floatvar1") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("floatvar2") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("nonexisting4") == false);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("stringvar2") == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsFunctionAvailable (Global)")
  {
    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting1") == false);
    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f1") == true);
    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f2") == true);
    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting2") == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetIntVariable (Global)")
  {
    PLASMA_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
    PLASMA_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetIntVariable (Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);
    PLASMA_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFloatVariable (Global)")
  {
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFloatVariable (Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, plMath::DefaultEpsilon<float>());
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, plMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetBoolVariable (Global)")
  {
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetBoolVariable (Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    PLASMA_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetStringVariable (Global)")
  {
    PLASMA_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
    PLASMA_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetStringVariable (Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");
    PLASMA_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (int, Global)")
  {
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    sMain.SetVariable("intvar1", 27);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 27);
    sMain.SetVariable("intvar1", 4);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (int, Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    sMain.SetVariable("intvar1", 127);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 127);
    sMain.SetVariable("intvar1", 14);
    PLASMA_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (float, Global)")
  {
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, plMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 27.3f);
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 27.3f, plMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 4.3f);
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, plMath::DefaultEpsilon<float>());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (float, Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, plMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 127.3f);
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 127.3f, plMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 14.3f);
    PLASMA_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, plMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (bool, Global)")
  {
    PLASMA_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    PLASMA_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    PLASMA_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (bool, Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    PLASMA_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    PLASMA_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (string, Global)")
  {
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "test1");
    sMain.SetVariable("stringvar1", "zweiundvierzig");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1", 3);
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "tes");
    sMain.SetVariable("stringvar1", "zweiundvierzigabc", 14);
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (string, Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+test1");
    sMain.SetVariable("stringvar1", "+zweiundvierzig");
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1", 4);
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+tes");
    sMain.SetVariable("stringvar1", "+zweiundvierzigabc", 15);
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (nil, Global)")
  {
    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "zweiundvierzig");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetVariable (nil, Table)")
  {
    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);

    PLASMA_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "+zweiundvierzig");

    sMain.CloseTable();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "OpenTable")
  {
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
    PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == true);
    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

    PLASMA_TEST_BOOL(sMain.OpenTable("NotMyTable") == PLASMA_FAILURE);

    PLASMA_TEST_BOOL(sMain.OpenTable("MyTable") == PLASMA_SUCCESS);
    {
      PLASMA_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      PLASMA_TEST_BOOL(sMain.OpenTable("NotMyTable") == PLASMA_FAILURE);

      PLASMA_TEST_BOOL(sMain.OpenTable("SubTable") == PLASMA_SUCCESS);
      {
        PLASMA_TEST_BOOL(sMain.OpenTable("NotMyTable") == PLASMA_FAILURE);

        PLASMA_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      PLASMA_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      PLASMA_TEST_BOOL(sMain.OpenTable("NotMyTable") == PLASMA_FAILURE);

      PLASMA_TEST_BOOL(sMain.OpenTable("SubTable") == PLASMA_SUCCESS);
      {
        PLASMA_TEST_BOOL(sMain.OpenTable("NotMyTable") == PLASMA_FAILURE);

        PLASMA_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        PLASMA_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      sMain.CloseTable();
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RegisterCFunction")
  {
    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == false);

    sMain.RegisterCFunction("Func1", MyFunc1);

    PLASMA_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Call Lua Function")
  {
    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("f_globaltable") == true);
    PLASMA_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == PLASMA_SUCCESS);

    ScriptLogIgnore::g_iErrors = 0;
    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("f_NotWorking") == true);
    PLASMA_TEST_BOOL(sMain.CallPreparedFunction(0, &LogIgnore) == PLASMA_FAILURE);
    PLASMA_TEST_INT(ScriptLogIgnore::g_iErrors, 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Call C Function")
  {
    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    if (sMain.IsFunctionAvailable("Func1") == false)
      sMain.RegisterCFunction("Func1", MyFunc1);

    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("Func1") == true);

    PLASMA_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == PLASMA_SUCCESS);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Call C Function with Parameters")
  {
    if (sMain.IsFunctionAvailable("Func2") == false)
      sMain.RegisterCFunction("Func2", MyFunc2);

    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("Func2") == true);

    sMain.PushParameter(true);
    sMain.PushParameter(2.3f);
    sMain.PushParameter(42);
    sMain.PushParameterNil();
    sMain.PushParameter("test");
    sMain.PushParameter("tuttut", 3);

    PLASMA_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == PLASMA_SUCCESS);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Call C Function with Return Values")
  {
    if (sMain.IsFunctionAvailable("Func3") == false)
      sMain.RegisterCFunction("Func3", MyFunc3);

    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    PLASMA_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(sMain.IsReturnValueBool(0));
    PLASMA_TEST_BOOL(sMain.IsReturnValueFloat(1));
    PLASMA_TEST_BOOL(sMain.IsReturnValueInt(2));
    PLASMA_TEST_BOOL(sMain.IsReturnValueNil(3));
    PLASMA_TEST_BOOL(sMain.IsReturnValueString(4));
    PLASMA_TEST_BOOL(sMain.IsReturnValueString(5));

    PLASMA_TEST_BOOL(sMain.GetBoolReturnValue(0) == false);
    PLASMA_TEST_FLOAT(sMain.GetFloatReturnValue(1), 2.3f, 0.0001f);
    PLASMA_TEST_INT(sMain.GetIntReturnValue(2), 42);
    PLASMA_TEST_STRING(sMain.GetStringReturnValue(4), "test");
    PLASMA_TEST_STRING(sMain.GetStringReturnValue(5), "tut");

    sMain.DiscardReturnValues();

    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    PLASMA_TEST_BOOL(sMain.CallPreparedFunction(6, &Log) == PLASMA_SUCCESS);

    sMain.DiscardReturnValues();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Call C Function with Table Parameter")
  {
    if (sMain.IsFunctionAvailable("Func4") == false)
      sMain.RegisterCFunction("Func4", MyFunc4);

    PLASMA_TEST_BOOL(sMain.PrepareFunctionCall("Func4") == true);

    sMain.PushTable("MyTable", true);

    PLASMA_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == PLASMA_SUCCESS);
  }
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
