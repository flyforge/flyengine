#include <CoreTest/CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeContext.h>

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static plResult TranspileString(const char* szSource, plDuktapeContext& ref_script, plStringBuilder& ref_sResult)
{
  ref_script.PushGlobalObject();                                           // [ global ]
  ref_script.PushLocalObject("ts").IgnoreResult();                         // [ global ts ]
  PLASMA_SUCCEED_OR_RETURN(ref_script.PrepareObjectFunctionCall("transpile")); // [ global ts transpile ]
  ref_script.PushString(szSource);                                         // [ global ts transpile source ]
  PLASMA_SUCCEED_OR_RETURN(ref_script.CallPreparedFunction());                 // [ global ts result ]
  ref_sResult = ref_script.GetStringValue(-1);                             // [ global ts result ]
  ref_script.PopStack(3);                                                  // [ ]

  return PLASMA_SUCCESS;
}

static plResult TranspileFile(const char* szFile, plDuktapeContext& ref_script, plStringBuilder& ref_sResult)
{
  plFileReader file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(szFile));

  plStringBuilder source;
  source.ReadAll(file);

  return TranspileString(source, ref_script, ref_sResult);
}

static plResult TranspileFileToJS(const char* szFile, plDuktapeContext& ref_script, plStringBuilder& ref_sResult)
{
  PLASMA_SUCCEED_OR_RETURN(TranspileFile(szFile, ref_script, ref_sResult));

  plStringBuilder sFile(":TypeScriptTest/", szFile);
  sFile.ChangeFileExtension("js");

  plFileWriter file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(sFile));

  PLASMA_SUCCEED_OR_RETURN(file.WriteBytes(ref_sResult.GetData(), ref_sResult.GetElementCount()));
  return PLASMA_SUCCESS;
}

static int Duk_Print(duk_context* pContext)
{
  plDuktapeFunction duk(pContext);

  plLog::Info(duk.GetStringValue(0));

  return duk.ReturnVoid();
}

static duk_ret_t ModuleSearchFunction2(duk_context* pCtx)
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

PLASMA_CREATE_SIMPLE_TEST(Scripting, TypeScript)
{
  // setup file system
  {
    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FolderType::Factory);

    plStringBuilder sTestDataDir(">sdk/", plTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/TypeScript");
    if (!PLASMA_TEST_RESULT(plFileSystem::AddDataDirectory(sTestDataDir, "TypeScriptTest", "TypeScriptTest", plFileSystem::AllowWrites)))
      return;

    if (!PLASMA_TEST_RESULT(plFileSystem::AddDataDirectory(">sdk/Data/Tools/PlasmaEditor", "DuktapeTest")))
      return;
  }

  plDuktapeContext duk("DukTS");
  duk.EnableModuleSupport(ModuleSearchFunction2);

  duk.RegisterGlobalFunction("Print", Duk_Print, 1);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compile TypeScriptServices") { PLASMA_TEST_RESULT(duk.ExecuteFile("Typescript/typescriptServices.js")); }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transpile Simple")
  {
    // simple way
    PLASMA_TEST_RESULT(duk.ExecuteString("ts.transpile('class X{}');"));

    // complicated way, needed to retrieve the result
    plStringBuilder sTranspiled;
    TranspileString("class X{}", duk, sTranspiled).IgnoreResult();

    // validate that the transpiled code can be executed by Duktape
    plDuktapeContext duk2("duk");
    PLASMA_TEST_RESULT(duk2.ExecuteString(sTranspiled));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Transpile File")
  {
    plStringBuilder result;
    PLASMA_TEST_RESULT(TranspileFileToJS("Foo.ts", duk, result));

    duk.ExecuteFile("Foo.js").IgnoreResult();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Import files")
  {
    plStringBuilder result;
    PLASMA_TEST_RESULT(TranspileFileToJS("Bar.ts", duk, result));

    duk.ExecuteFile("Bar.js").IgnoreResult();
  }

  plFileSystem::DeleteFile(":TypeScriptTest/Foo.js");
  plFileSystem::DeleteFile(":TypeScriptTest/Bar.js");

  plFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

#endif
