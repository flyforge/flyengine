#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/Profiling/Profiling.h>
#include <TestFramework/Framework/TestFramework.h>

PLASMA_ENUMERABLE_CLASS_IMPLEMENTATION(plRegisterSimpleTestHelper);

void plSimpleTestGroup::AddSimpleTest(const char* szName, SimpleTestFunc testFunc)
{
  SimpleTestEntry e;
  e.m_szName = szName;
  e.m_Func = testFunc;

  for (plUInt32 i = 0; i < m_SimpleTests.size(); ++i)
  {
    if ((strcmp(m_SimpleTests[i].m_szName, e.m_szName) == 0) && (m_SimpleTests[i].m_Func == e.m_Func))
      return;
  }

  m_SimpleTests.push_back(e);
}

void plSimpleTestGroup::SetupSubTests()
{
  for (plUInt32 i = 0; i < m_SimpleTests.size(); ++i)
  {
    AddSubTest(m_SimpleTests[i].m_szName, i);
  }
}

plTestAppRun plSimpleTestGroup::RunSubTest(plInt32 iIdentifier, plUInt32 uiInvocationCount)
{
  // until the block name is properly set, use the test name instead
  plTestFramework::s_szTestBlockName = m_SimpleTests[iIdentifier].m_szName;

  PLASMA_PROFILE_SCOPE(m_SimpleTests[iIdentifier].m_szName);
  m_SimpleTests[iIdentifier].m_Func();

  plTestFramework::s_szTestBlockName = "";
  return plTestAppRun::Quit;
}

plResult plSimpleTestGroup::InitializeSubTest(plInt32 iIdentifier)
{
  // initialize everything up to 'core'
  plStartup::StartupCoreSystems();
  return PLASMA_SUCCESS;
}

plResult plSimpleTestGroup::DeInitializeSubTest(plInt32 iIdentifier)
{
  // shut down completely
  plStartup::ShutdownCoreSystems();
  plMemoryTracker::DumpMemoryLeaks();
  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_SimpleTest);
