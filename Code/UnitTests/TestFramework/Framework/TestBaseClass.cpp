#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Framework/TestFramework.h>

PLASMA_ENUMERABLE_CLASS_IMPLEMENTATION(plTestBaseClass);

const char* plTestBaseClass::GetSubTestName(plInt32 iIdentifier) const
{
  if (iIdentifier < 0 || static_cast<std::size_t>(iIdentifier) > m_Entries.size())
  {
    plLog::Error("Tried to access retrieve sub-test name using invalid identifier.");
    return "";
  }

  return m_Entries[iIdentifier].m_szName;
}

void plTestBaseClass::UpdateConfiguration(plTestConfiguration& ref_config) const
{
  // If the configuration hasn't been set yet this is the first instance of plTestBaseClass being called
  // to fill in the configuration and we thus have to do so.
  // Derived classes can have more information (e.g.GPU info) and there is no way to know which instance
  // of plTestBaseClass may have additional information so we ask all of them and each one early outs
  // if the information it knows about is already present.
  if (ref_config.m_uiInstalledMainMemory == 0)
  {
    const plSystemInformation& pSysInfo = plSystemInformation::Get();
    ref_config.m_uiInstalledMainMemory = pSysInfo.GetInstalledMainMemory();
    ref_config.m_uiMemoryPageSize = pSysInfo.GetMemoryPageSize();
    ref_config.m_uiCPUCoreCount = pSysInfo.GetCPUCoreCount();
    ref_config.m_sPlatformName = pSysInfo.GetPlatformName();
    ref_config.m_b64BitOS = pSysInfo.Is64BitOS();
    ref_config.m_b64BitApplication = PLASMA_ENABLED(PLASMA_PLATFORM_64BIT);
    ref_config.m_sBuildConfiguration = pSysInfo.GetBuildConfiguration();
    ref_config.m_iDateTime = plTimestamp::CurrentTimestamp().GetInt64(plSIUnitOfTime::Second);
    ref_config.m_iRCSRevision = plTestFramework::GetInstance()->GetSettings().m_iRevision;
    ref_config.m_sHostName = pSysInfo.GetHostName();
  }
}

void plTestBaseClass::MapImageNumberToString(
  const char* szTestName, const char* szSubTestName, plUInt32 uiImageNumber, plStringBuilder& out_sString) const
{
  out_sString.Format("{0}_{1}_{2}", szTestName, szSubTestName, plArgI(uiImageNumber, 3, true));
  out_sString.ReplaceAll(" ", "_");
}

void plTestBaseClass::ClearSubTests()
{
  m_Entries.clear();
}

void plTestBaseClass::AddSubTest(const char* szName, plInt32 iIdentifier)
{
  PLASMA_ASSERT_DEV(szName != nullptr, "Sub test name must not be nullptr");

  TestEntry e;
  e.m_szName = szName;
  e.m_iIdentifier = iIdentifier;

  m_Entries.push_back(e);
}

plResult plTestBaseClass::DoTestInitialization()
{
  try
  {
    if (InitializeTest() == PLASMA_FAILURE)
    {
      plTestFramework::Output(plTestOutput::Error, "Test Initialization failed.");
      return PLASMA_FAILURE;
    }
  }
  catch (...)
  {
    plTestFramework::Output(plTestOutput::Error, "Exception during test initialization.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

void plTestBaseClass::DoTestDeInitialization()
{
  try

  {
    if (DeInitializeTest() == PLASMA_FAILURE)
      plTestFramework::Output(plTestOutput::Error, "Test DeInitialization failed.");
  }
  catch (...)
  {
    plTestFramework::Output(plTestOutput::Error, "Exception during test de-initialization.");
  }
}

plResult plTestBaseClass::DoSubTestInitialization(plInt32 iIdentifier)
{
  try
  {
    if (InitializeSubTest(iIdentifier) == PLASMA_FAILURE)
    {
      plTestFramework::Output(plTestOutput::Error, "Sub-Test Initialization failed, skipping Test.");
      return PLASMA_FAILURE;
    }
  }
  catch (...)
  {
    plTestFramework::Output(plTestOutput::Error, "Exception during sub-test initialization.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

void plTestBaseClass::DoSubTestDeInitialization(plInt32 iIdentifier)
{
  try
  {
    if (DeInitializeSubTest(iIdentifier) == PLASMA_FAILURE)
      plTestFramework::Output(plTestOutput::Error, "Sub-Test De-Initialization failed.");
  }
  catch (...)
  {
    plTestFramework::Output(plTestOutput::Error, "Exception during sub-test de-initialization.");
  }
}

plTestAppRun plTestBaseClass::DoSubTestRun(plInt32 iIdentifier, double& fDuration, plUInt32 uiInvocationCount)
{
  fDuration = 0.0;

  plTestAppRun ret = plTestAppRun::Quit;

  try
  {
    plTime StartTime = plTime::Now();

    ret = RunSubTest(iIdentifier, uiInvocationCount);

    fDuration = (plTime::Now() - StartTime).GetMilliseconds();
  }
  catch (...)
  {
    plInt32 iEntry = -1;

    for (plInt32 i = 0; i < (plInt32)m_Entries.size(); ++i)
    {
      if (m_Entries[i].m_iIdentifier == iIdentifier)
      {
        iEntry = i;
        break;
      }
    }

    if (iEntry >= 0)
      plTestFramework::Output(plTestOutput::Error, "Exception during sub-test '%s'.", m_Entries[iEntry].m_szName);
    else
      plTestFramework::Output(plTestOutput::Error, "Exception during unknown sub-test.");
  }

  return ret;
}


PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestBaseClass);
