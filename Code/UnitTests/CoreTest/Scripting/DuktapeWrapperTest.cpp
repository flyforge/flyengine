#include <CoreTest/CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeContext.h>

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static duk_ret_t ModuleSearchFunction(duk_context* pCtx);

static int CFuncPrint(duk_context* pContext)
{
  plDuktapeFunction wrapper(pContext);
  const char* szText = wrapper.GetStringValue(0, nullptr);

  plLog::Info("Print: '{}'", szText);
  return wrapper.ReturnVoid();
}

static int CFuncPrintVA(duk_context* pContext)
{
  plDuktapeFunction wrapper(pContext);

  const plUInt32 uiNumArgs = wrapper.GetNumVarArgFunctionParameters();

  plStringBuilder s;
  s.AppendFormat("#Args: {}", uiNumArgs);

  for (plUInt32 arg = 0; arg < uiNumArgs; ++arg)
  {
    if (wrapper.IsNumber(arg))
    {
      double val = wrapper.GetNumberValue(arg);
      s.AppendFormat(", #{}: Number = {}", arg, val);
    }
    else if (wrapper.IsBool(arg))
    {
      bool val = wrapper.GetBoolValue(arg);
      s.AppendFormat(", #{}: Bool = {}", arg, val);
    }
    else if (wrapper.IsString(arg))
    {
      const char* val = wrapper.GetStringValue(arg);
      s.AppendFormat(", #{}: String = {}", arg, val);
    }
    else if (wrapper.IsNull(arg))
    {
      s.AppendFormat(", #{}: null", arg);
    }
    else if (wrapper.IsUndefined(arg))
    {
      s.AppendFormat(", #{}: undefined", arg);
    }
    else if (wrapper.IsObject(arg))
    {
      s.AppendFormat(", #{}: object", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_BUFFER))
    {
      s.AppendFormat(", #{}: buffer", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_POINTER))
    {
      s.AppendFormat(", #{}: pointer", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_LIGHTFUNC))
    {
      s.AppendFormat(", #{}: lightfunc", arg);
    }
    else
    {
      s.AppendFormat(", #{}: UNKNOWN TYPE", arg);
    }
  }

  plLog::Info(s);
  return wrapper.ReturnString(s);
}

static int CFuncMagic(duk_context* pContext)
{
  plDuktapeFunction wrapper(pContext);
  plInt16 iMagic = wrapper.GetFunctionMagicValue();

  plLog::Info("Magic: '{}'", iMagic);
  return wrapper.ReturnInt(iMagic);
}


PLASMA_CREATE_SIMPLE_TEST(Scripting, DuktapeWrapper)
{
  // setup file system
  {
    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FolderType::Factory);

    plStringBuilder sTestDataDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/Duktape");
    if (!PLASMA_TEST_RESULT(plFileSystem::AddDataDirectory(sTestDataDir, "DuktapeTest")))
      return;
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basics")
  {
    plDuktapeContext duk("DukTest");

    duk_eval_string(duk.GetContext(), "'testString'.toUpperCase()");
    plStringBuilder sTestString = duk_get_string(duk.GetContext(), -1);
    duk_pop(duk.GetContext());
    PLASMA_TEST_STRING(sTestString, "TESTSTRING");

    PLASMA_TEST_RESULT(duk.ExecuteString("function MakeUpper(bla) { return bla.toUpperCase() }"));


    duk_eval_string(duk.GetContext(), "MakeUpper(\"myTest\")");
    sTestString = duk_get_string(duk.GetContext(), -1);
    duk_pop(duk.GetContext());
    PLASMA_TEST_STRING(sTestString, "MYTEST");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExecuteString (error)")
  {
    plDuktapeContext duk("DukTest");

    plTestLogInterface log;
    plTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("SyntaxError: parse error (line 1)", plLogMsgType::ErrorMsg);
    PLASMA_TEST_BOOL(duk.ExecuteString(" == invalid code == ").Failed());

    log.ExpectMessage("ReferenceError: identifier 'Print' undefined", plLogMsgType::ErrorMsg);
    PLASMA_TEST_BOOL(duk.ExecuteString("Print(\"do stuff\")").Failed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExecuteFile")
  {
    plDuktapeContext duk("DukTest");
    duk.EnableModuleSupport(nullptr);

    plTestLogInterface log;
    plTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Print: 'called f1'", plLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    duk.ExecuteFile("ExecuteFile.js").IgnoreResult();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "C Function")
  {
    plDuktapeContext duk("DukTest");

    plTestLogInterface log;
    plTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Hello Test", plLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    duk.ExecuteString("Print('Hello Test')").IgnoreResult();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "VarArgs C Function")
  {
    plDuktapeContext duk("DukTest");

    plTestLogInterface log;
    plTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("#Args: 5, #0: String = text, #1: Number = 7, #2: Bool = true, #3: null, #4: object", plLogMsgType::InfoMsg);

    duk.RegisterGlobalFunctionWithVarArgs("PrintVA", CFuncPrintVA);

    duk.ExecuteString("PrintVA('text', 7, true, null, {})").IgnoreResult();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Call Function")
  {
    plDuktapeContext duk("DukTest");

    plTestLogInterface log;
    plTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("You did it", plLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    if (PLASMA_TEST_RESULT(duk.PrepareGlobalFunctionCall("Print"))) // [ Print ] / [ ]
    {
      duk.PushString("You did it, Fry!");         // [ Print String ]
      PLASMA_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                             // [ ]
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Function Magic Value")
  {
    plDuktapeContext duk("DukTest");

    plTestLogInterface log;
    plTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Magic: '1'", plLogMsgType::InfoMsg);
    log.ExpectMessage("Magic: '2'", plLogMsgType::InfoMsg);
    log.ExpectMessage("Magic: '3'", plLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Magic1", CFuncMagic, 0, 1);
    duk.RegisterGlobalFunction("Magic2", CFuncMagic, 0, 2);
    duk.RegisterGlobalFunction("Magic3", CFuncMagic, 0, 3);

    if (PLASMA_TEST_RESULT(duk.PrepareGlobalFunctionCall("Magic1"))) // [ Magic1 ]
    {
      PLASMA_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                             // [ ]
    }

    if (PLASMA_TEST_RESULT(duk.PrepareGlobalFunctionCall("Magic2"))) // [ Magic2 ]
    {
      PLASMA_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                             // [ ]
    }

    if (PLASMA_TEST_RESULT(duk.PrepareGlobalFunctionCall("Magic3"))) // [ Magic2 ]
    {
      PLASMA_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                             // [ ]
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Inspect Object")
  {
    plDuktapeContext duk("DukTest");
    plDuktapeHelper val(duk);
    PLASMA_TEST_RESULT(duk.ExecuteFile("Object.js"));

    duk.PushGlobalObject();                     // [ global ]
    PLASMA_TEST_RESULT(duk.PushLocalObject("obj")); // [ global obj ]

    PLASMA_TEST_BOOL(duk.HasProperty("i"));
    PLASMA_TEST_INT(duk.GetIntProperty("i", 0), 23);

    PLASMA_TEST_BOOL(duk.HasProperty("f"));
    PLASMA_TEST_FLOAT(duk.GetFloatProperty("f", 0), 4.2f, 0.01f);

    PLASMA_TEST_BOOL(duk.HasProperty("b"));
    PLASMA_TEST_BOOL(duk.GetBoolProperty("b", false));

    PLASMA_TEST_BOOL(duk.HasProperty("s"));
    PLASMA_TEST_STRING(duk.GetStringProperty("s", ""), "text");

    PLASMA_TEST_BOOL(duk.HasProperty("n"));

    PLASMA_TEST_BOOL(duk.HasProperty("o"));

    {
      PLASMA_TEST_RESULT(duk.PushLocalObject("o")); // [ global obj o ]
      PLASMA_TEST_BOOL(duk.HasProperty("sub"));
      PLASMA_TEST_STRING(duk.GetStringProperty("sub", ""), "wub");
    }

    duk.PopStack(3); // [ ]
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "require")
  {
    plDuktapeContext duk("DukTest");
    duk.EnableModuleSupport(ModuleSearchFunction);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    plTestLogInterface log;
    plTestLogSystemScope logSystemScope(&log);
    log.ExpectMessage("Print: 'called f1'", plLogMsgType::InfoMsg);
    log.ExpectMessage("Print: 'Called require.js'", plLogMsgType::InfoMsg);
    log.ExpectMessage("Print: 'require.js: called f1'", plLogMsgType::InfoMsg);

    PLASMA_TEST_RESULT(duk.ExecuteFile("require.js"));
  }

  plFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

static duk_ret_t ModuleSearchFunction(duk_context* pCtx)
{
  plDuktapeFunction script(pCtx);

  /* Nargs was given as 4 and we get the following stack arguments:
   *   index 0: id
   *   index 1: require
   *   index 2: exports
   *   index 3: module
   */

  plStringBuilder id = script.GetStringValue(0);
  id.ChangeFileExtension("js");

  plStringBuilder source;
  plFileReader file;
  file.Open(id).IgnoreResult();
  source.ReadAll(file);

  return script.ReturnString(source);

  /* Return 'undefined' to indicate no source code. */
  // return 0;
}

#endif
