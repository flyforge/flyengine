#include <GameEngineTest/GameEngineTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include "TypeScriptTest.h"
#  include <Core/Messages/CommonMessages.h>
#  include <Core/Messages/EventMessage.h>
#  include <Core/Scripting/DuktapeFunction.h>
#  include <Core/Scripting/DuktapeHelper.h>
#  include <Core/WorldSerializer/WorldReader.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <TypeScriptPlugin/Components/TypeScriptComponent.h>

static plGameEngineTestTypeScript s_GameEngineTestTypeScript;

const char* plGameEngineTestTypeScript::GetTestName() const
{
  return "TypeScript Tests";
}

plGameEngineTestApplication* plGameEngineTestTypeScript::CreateApplication()
{
  m_pOwnApplication = PLASMA_DEFAULT_NEW(plGameEngineTestApplication_TypeScript);
  return m_pOwnApplication;
}

void plGameEngineTestTypeScript::SetupSubTests()
{
  AddSubTest("Vec2", SubTests::Vec2);
  AddSubTest("Vec3", SubTests::Vec3);
  AddSubTest("Quat", SubTests::Quat);
  AddSubTest("Mat3", SubTests::Mat3);
  AddSubTest("Mat4", SubTests::Mat4);
  AddSubTest("Transform", SubTests::Transform);
  AddSubTest("Color", SubTests::Color);
  AddSubTest("Debug", SubTests::Debug);
  AddSubTest("GameObject", SubTests::GameObject);
  AddSubTest("Component", SubTests::Component);
  AddSubTest("Lifetime", SubTests::Lifetime);
  AddSubTest("Messaging", SubTests::Messaging);
  AddSubTest("World", SubTests::World);
  AddSubTest("Utils", SubTests::Utils);
}

plResult plGameEngineTestTypeScript::InitializeSubTest(plInt32 iIdentifier)
{
  m_pOwnApplication->SubTestBasicsSetup();
  return PLASMA_SUCCESS;
}

plTestAppRun plGameEngineTestTypeScript::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  return m_pOwnApplication->SubTestBasisExec(GetSubTestName(iIdentifier));
}

//////////////////////////////////////////////////////////////////////////

plGameEngineTestApplication_TypeScript::plGameEngineTestApplication_TypeScript()
  : plGameEngineTestApplication("TypeScript")
{
}

//////////////////////////////////////////////////////////////////////////

static int Duk_TestFailure(duk_context* pDuk)
{
  plDuktapeFunction duk(pDuk);

  const char* szFile = duk.GetStringValue(0);
  const plInt32 iLine = duk.GetIntValue(1);
  const char* szFunction = duk.GetStringValue(2);
  const char* szMsg = duk.GetStringValue(3);

  plTestBool(false, "TypeScript Test Failed", szFile, iLine, szFunction, szMsg);

  return duk.ReturnVoid();
}

void plGameEngineTestApplication_TypeScript::SubTestBasicsSetup()
{
  LoadScene("TypeScript/AssetCache/Common/Scenes/TypeScripting.plObjectGraph").IgnoreResult();

  PLASMA_LOCK(m_pWorld->GetWriteMarker());
  plTypeScriptComponentManager* pMan = m_pWorld->GetOrCreateComponentManager<plTypeScriptComponentManager>();

  pMan->GetTsBinding().GetDukTapeContext().RegisterGlobalFunction("plTestFailure", Duk_TestFailure, 4);
}

plTestAppRun plGameEngineTestApplication_TypeScript::SubTestBasisExec(const char* szSubTestName)
{
  if (Run() == plApplication::Execution::Quit)
    return plTestAppRun::Quit;

  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  plGameObject* pTests = nullptr;
  if (m_pWorld->TryGetObjectWithGlobalKey("Tests", pTests) == false)
  {
    PLASMA_TEST_FAILURE("Failed to retrieve TypeScript Tests-Object", "");
    return plTestAppRun::Quit;
  }

  const plStringBuilder sMsg("Test", szSubTestName);

  plMsgGenericEvent msg;
  msg.m_sMessage.Assign(sMsg);
  pTests->SendMessageRecursive(msg);

  if (msg.m_sMessage == plTempHashedString("repeat"))
    return plTestAppRun::Continue;

  PLASMA_TEST_STRING(msg.m_sMessage, "done");

  return plTestAppRun::Quit;
}

#endif
