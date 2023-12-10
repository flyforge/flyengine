#include <TestFramework/TestFrameworkPCH.h>

#include <Foundation/Types/ScopeExit.h>
#include <TestFramework/Framework/TestResults.h>

////////////////////////////////////////////////////////////////////////
// plTestOutput public functions
////////////////////////////////////////////////////////////////////////

const char* const plTestOutput::s_Names[] = {
  "StartOutput", "BeginBlock", "EndBlock", "ImportantInfo", "Details", "Success", "Message", "Warning", "Error", "Duration", "FinalResult"};

const char* plTestOutput::ToString(Enum type)
{
  return s_Names[type];
}

plTestOutput::Enum plTestOutput::FromString(const char* szName)
{
  for (plUInt32 i = 0; i < AllOutputTypes; ++i)
  {
    if (strcmp(szName, s_Names[i]) == 0)
      return (plTestOutput::Enum)i;
  }
  return InvalidType;
}


////////////////////////////////////////////////////////////////////////
// plTestResultData public functions
////////////////////////////////////////////////////////////////////////

void plTestResultData::Reset()
{
  m_bExecuted = false;
  m_bSuccess = false;
  m_iTestAsserts = 0;
  m_fTestDuration = 0.0;
  m_iFirstOutput = -1;
  m_iLastOutput = -1;
}

void plTestResultData::AddOutput(plInt32 iOutputIndex)
{
  if (m_iFirstOutput == -1)
  {
    m_iFirstOutput = iOutputIndex;
    m_iLastOutput = iOutputIndex;
  }
  else
  {
    m_iLastOutput = iOutputIndex;
  }
}


////////////////////////////////////////////////////////////////////////
// plTestResultData public functions
////////////////////////////////////////////////////////////////////////

plTestConfiguration::plTestConfiguration()

  = default;


////////////////////////////////////////////////////////////////////////
// plTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void plTestFrameworkResult::Clear()
{
  m_Tests.clear();
  m_Errors.clear();
  m_TestOutput.clear();
}

void plTestFrameworkResult::SetupTests(const std::deque<plTestEntry>& tests, const plTestConfiguration& config)
{
  m_Config = config;
  Clear();

  const plUInt32 uiTestCount = (plUInt32)tests.size();
  for (plUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests.push_back(plTestResult(tests[uiTestIdx].m_szTestName));

    const plUInt32 uiSubTestCount = (plUInt32)tests[uiTestIdx].m_SubTests.size();
    for (plUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTestCount; ++uiSubTestIdx)
    {
      m_Tests[uiTestIdx].m_SubTests.push_back(plSubTestResult(tests[uiTestIdx].m_SubTests[uiSubTestIdx].m_szSubTestName));
    }
  }
}

void ::plTestFrameworkResult::Reset()
{
  const plUInt32 uiTestCount = (plUInt32)m_Tests.size();
  for (plUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_Tests[uiTestIdx].Reset();
  }
  m_Errors.clear();
  m_TestOutput.clear();
}

bool plTestFrameworkResult::WriteJsonToFile(const char* szFileName) const
{
  plStartup::StartupCoreSystems();
  PLASMA_SCOPE_EXIT(plStartup::ShutdownCoreSystems());

  {
    plStringBuilder jsonFilename;
    if (plPathUtils::IsAbsolutePath(szFileName))
    {
      // Make sure we can access raw absolute file paths
      if (plFileSystem::AddDataDirectory("", "jsonoutput", ":", plFileSystem::AllowWrites).Failed())
        return false;

      jsonFilename = szFileName;
    }
    else
    {
      // If this is a relative path, we use the pltest/ data directory to make sure that this works properly with the fileserver.
      if (plFileSystem::AddDataDirectory(">pltest/", "jsonoutput", ":", plFileSystem::AllowWrites).Failed())
        return false;

      jsonFilename = ":";
      jsonFilename.AppendPath(szFileName);
    }

    plFileWriter file;
    if (file.Open(jsonFilename).Failed())
    {
      return false;
    }
    plStandardJSONWriter js;
    js.SetOutputStream(&file);

    js.BeginObject();
    {
      js.BeginObject("configuration");
      {
        js.AddVariableUInt64("m_uiInstalledMainMemory", m_Config.m_uiInstalledMainMemory);
        js.AddVariableUInt32("m_uiMemoryPageSize", m_Config.m_uiMemoryPageSize);
        js.AddVariableUInt32("m_uiCPUCoreCount", m_Config.m_uiCPUCoreCount);
        js.AddVariableBool("m_b64BitOS", m_Config.m_b64BitOS);
        js.AddVariableBool("m_b64BitApplication", m_Config.m_b64BitApplication);
        js.AddVariableString("m_sPlatformName", m_Config.m_sPlatformName.c_str());
        js.AddVariableString("m_sBuildConfiguration", m_Config.m_sBuildConfiguration.c_str());
        js.AddVariableInt64("m_iDateTime", m_Config.m_iDateTime);
        js.AddVariableInt32("m_iRCSRevision", m_Config.m_iRCSRevision);
        js.AddVariableString("m_sHostName", m_Config.m_sHostName.c_str());
      }
      js.EndObject();

      // Output Messages
      js.BeginArray("messages");
      {
        plUInt32 uiMessages = GetOutputMessageCount();
        for (plUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const plTestOutputMessage* pMessage = GetOutputMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_Type", plTestOutput::ToString(pMessage->m_Type));
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
            if (pMessage->m_iErrorIndex != -1)
              js.AddVariableInt32("m_iErrorIndex", pMessage->m_iErrorIndex);
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Error Messages
      js.BeginArray("errors");
      {
        plUInt32 uiMessages = GetErrorMessageCount();
        for (plUInt32 uiMessageIdx = 0; uiMessageIdx < uiMessages; ++uiMessageIdx)
        {
          const plTestErrorMessage* pMessage = GetErrorMessage(uiMessageIdx);
          js.BeginObject();
          {
            js.AddVariableString("m_sError", pMessage->m_sError.c_str());
            js.AddVariableString("m_sBlock", pMessage->m_sBlock.c_str());
            js.AddVariableString("m_sFile", pMessage->m_sFile.c_str());
            js.AddVariableString("m_sFunction", pMessage->m_sFunction.c_str());
            js.AddVariableInt32("m_iLine", pMessage->m_iLine);
            js.AddVariableString("m_sMessage", pMessage->m_sMessage.c_str());
          }
          js.EndObject();
        }
      }
      js.EndArray();

      // Tests
      js.BeginArray("tests");
      {
        plUInt32 uiTests = GetTestCount();
        for (plUInt32 uiTestIdx = 0; uiTestIdx < uiTests; ++uiTestIdx)
        {
          const plTestResultData& testResult = GetTestResultData(uiTestIdx, -1);
          js.BeginObject();
          {
            js.AddVariableString("m_sName", testResult.m_sName.c_str());
            js.AddVariableBool("m_bExecuted", testResult.m_bExecuted);
            js.AddVariableBool("m_bSuccess", testResult.m_bSuccess);
            js.AddVariableInt32("m_iTestAsserts", testResult.m_iTestAsserts);
            js.AddVariableDouble("m_fTestDuration", testResult.m_fTestDuration);
            js.AddVariableInt32("m_iFirstOutput", testResult.m_iFirstOutput);
            js.AddVariableInt32("m_iLastOutput", testResult.m_iLastOutput);

            // Sub Tests
            js.BeginArray("subTests");
            {
              plUInt32 uiSubTests = GetSubTestCount(uiTestIdx);
              for (plUInt32 uiSubTestIdx = 0; uiSubTestIdx < uiSubTests; ++uiSubTestIdx)
              {
                const plTestResultData& subTestResult = GetTestResultData(uiTestIdx, uiSubTestIdx);
                js.BeginObject();
                {
                  js.AddVariableString("m_sName", subTestResult.m_sName.c_str());
                  js.AddVariableBool("m_bExecuted", subTestResult.m_bExecuted);
                  js.AddVariableBool("m_bSuccess", subTestResult.m_bSuccess);
                  js.AddVariableInt32("m_iTestAsserts", subTestResult.m_iTestAsserts);
                  js.AddVariableDouble("m_fTestDuration", subTestResult.m_fTestDuration);
                  js.AddVariableInt32("m_iFirstOutput", subTestResult.m_iFirstOutput);
                  js.AddVariableInt32("m_iLastOutput", subTestResult.m_iLastOutput);
                }
                js.EndObject();
              }
            }
            js.EndArray(); // subTests
          }
          js.EndObject();
        }
      }
      js.EndArray(); // tests
    }
    js.EndObject();
  }

  return true;
}

plUInt32 plTestFrameworkResult::GetTestCount(plTestResultQuery::Enum countQuery) const
{
  plUInt32 uiAccumulator = 0;
  const plUInt32 uiTests = (plUInt32)m_Tests.size();

  if (countQuery == plTestResultQuery::Count)
    return uiTests;

  if (countQuery == plTestResultQuery::Errors)
    return (plUInt32)m_Errors.size();

  for (plUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    switch (countQuery)
    {
      case plTestResultQuery::Executed:
        uiAccumulator += m_Tests[uiTest].m_Result.m_bExecuted ? 1 : 0;
        break;
      case plTestResultQuery::Success:
        uiAccumulator += m_Tests[uiTest].m_Result.m_bSuccess ? 1 : 0;
        break;
      default:
        break;
    }
  }
  return uiAccumulator;
}

plUInt32 plTestFrameworkResult::GetSubTestCount(plUInt32 uiTestIndex, plTestResultQuery::Enum countQuery) const
{
  if (uiTestIndex >= (plUInt32)m_Tests.size())
    return 0;

  const plTestResult& test = m_Tests[uiTestIndex];
  plUInt32 uiAccumulator = 0;
  const plUInt32 uiSubTests = (plUInt32)test.m_SubTests.size();

  if (countQuery == plTestResultQuery::Count)
    return uiSubTests;

  if (countQuery == plTestResultQuery::Errors)
  {
    for (plInt32 iOutputIdx = test.m_Result.m_iFirstOutput; iOutputIdx <= test.m_Result.m_iLastOutput && iOutputIdx != -1; ++iOutputIdx)
    {
      if (m_TestOutput[iOutputIdx].m_Type == plTestOutput::Error)
        uiAccumulator++;
    }
    return uiAccumulator;
  }

  for (plUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    switch (countQuery)
    {
      case plTestResultQuery::Executed:
        uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bExecuted ? 1 : 0;
        break;
      case plTestResultQuery::Success:
        uiAccumulator += test.m_SubTests[uiSubTest].m_Result.m_bSuccess ? 1 : 0;
        break;
      default:
        break;
    }
  }
  return uiAccumulator;
}

plInt32 plTestFrameworkResult::GetTestIndexByName(const char* szTestName) const
{
  plInt32 iTestCount = (plInt32)GetTestCount();
  for (plInt32 i = 0; i < iTestCount; ++i)
  {
    if (m_Tests[i].m_Result.m_sName.compare(szTestName) == 0)
      return i;
  }
  return -1;
}

plInt32 plTestFrameworkResult::GetSubTestIndexByName(plUInt32 uiTestIndex, const char* szSubTestName) const
{
  if (uiTestIndex >= GetTestCount())
    return -1;

  plInt32 iSubTestCount = (plInt32)GetSubTestCount(uiTestIndex);
  for (plInt32 i = 0; i < iSubTestCount; ++i)
  {
    if (m_Tests[uiTestIndex].m_SubTests[i].m_Result.m_sName.compare(szSubTestName) == 0)
      return i;
  }
  return -1;
}

double plTestFrameworkResult::GetTotalTestDuration() const
{
  double fTotalTestDuration = 0.0;
  const plUInt32 uiTests = (plUInt32)m_Tests.size();
  for (plUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    fTotalTestDuration += m_Tests[uiTest].m_Result.m_fTestDuration;
  }
  return fTotalTestDuration;
}

const plTestResultData& plTestFrameworkResult::GetTestResultData(plUInt32 uiTestIndex, plInt32 iSubTestIndex) const
{
  return (iSubTestIndex == -1) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result;
}

void plTestFrameworkResult::TestOutput(plUInt32 uiTestIndex, plInt32 iSubTestIndex, plTestOutput::Enum type, const char* szMsg)
{
  if (uiTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.AddOutput((plInt32)m_TestOutput.size());
    if (iSubTestIndex != -1)
    {
      m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result.AddOutput((plInt32)m_TestOutput.size());
    }
  }

  m_TestOutput.push_back(plTestOutputMessage());
  plTestOutputMessage& outputMessage = *m_TestOutput.rbegin();
  outputMessage.m_Type = type;
  outputMessage.m_sMessage.assign(szMsg);
}

void plTestFrameworkResult::TestError(plUInt32 uiTestIndex, plInt32 iSubTestIndex, const char* szError, const char* szBlock, const char* szFile,
  plInt32 iLine, const char* szFunction, const char* szMsg)
{
  // In case there is no message set, we use the error as the message.
  TestOutput(uiTestIndex, iSubTestIndex, plTestOutput::Error, szError);
  m_TestOutput.rbegin()->m_iErrorIndex = (plInt32)m_Errors.size();

  m_Errors.push_back(plTestErrorMessage());
  plTestErrorMessage& errorMessage = *m_Errors.rbegin();
  errorMessage.m_sError.assign(szError);
  errorMessage.m_sBlock.assign(szBlock);
  errorMessage.m_sFile.assign(szFile);
  errorMessage.m_iLine = iLine;
  errorMessage.m_sFunction.assign(szFunction);
  errorMessage.m_sMessage.assign(szMsg);
}

void plTestFrameworkResult::TestResult(plUInt32 uiTestIndex, plInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  plTestResultData& Result = (iSubTestIndex == -1) ? m_Tests[uiTestIndex].m_Result : m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result;

  Result.m_bExecuted = true;
  Result.m_bSuccess = bSuccess;
  Result.m_fTestDuration = fDuration;

  // Accumulate sub-test duration onto test duration to get duration feedback while the sub-tests are running.
  // Final time will be set again once the entire test finishes and currently these times are identical as
  // init and de-init times aren't measured at the moment due to missing timer when engine is shut down.
  if (iSubTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.m_fTestDuration += fDuration;
  }
}

void plTestFrameworkResult::AddAsserts(plUInt32 uiTestIndex, plInt32 iSubTestIndex, int iCount)
{
  if (uiTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_Result.m_iTestAsserts += iCount;
  }

  if (iSubTestIndex != -1)
  {
    m_Tests[uiTestIndex].m_SubTests[iSubTestIndex].m_Result.m_iTestAsserts += iCount;
  }
}

plUInt32 plTestFrameworkResult::GetOutputMessageCount(plInt32 iTestIndex, plInt32 iSubTestIndex, plTestOutput::Enum type) const
{
  if (iTestIndex == -1 && type == plTestOutput::AllOutputTypes)
    return (plUInt32)m_TestOutput.size();

  plInt32 iStartIdx = 0;
  plInt32 iEndIdx = (plInt32)m_TestOutput.size() - 1;

  if (iTestIndex != -1)
  {
    const plTestResultData& result = GetTestResultData(iTestIndex, iSubTestIndex);
    iStartIdx = result.m_iFirstOutput;
    iEndIdx = result.m_iLastOutput;

    // If no messages have been output (yet) for the given test we early-out here.
    if (iStartIdx == -1)
      return 0;

    // If all message types should be counted we can simply return the range.
    if (type == plTestOutput::AllOutputTypes)
      return iEndIdx - iStartIdx + 1;
  }

  plUInt32 uiAccumulator = 0;
  for (plInt32 uiOutputMessageIdx = iStartIdx; uiOutputMessageIdx <= iEndIdx; ++uiOutputMessageIdx)
  {
    if (m_TestOutput[uiOutputMessageIdx].m_Type == type)
      uiAccumulator++;
  }
  return uiAccumulator;
}

const plTestOutputMessage* plTestFrameworkResult::GetOutputMessage(plUInt32 uiOutputMessageIdx) const
{
  return &m_TestOutput[uiOutputMessageIdx];
}

plUInt32 plTestFrameworkResult::GetErrorMessageCount(plInt32 iTestIndex, plInt32 iSubTestIndex) const
{
  // If no test is given we can simply return the total error count.
  if (iTestIndex == -1)
  {
    return (plUInt32)m_Errors.size();
  }

  return GetOutputMessageCount(iTestIndex, iSubTestIndex, plTestOutput::Error);
}

const plTestErrorMessage* plTestFrameworkResult::GetErrorMessage(plUInt32 uiErrorMessageIdx) const
{
  return &m_Errors[uiErrorMessageIdx];
}


////////////////////////////////////////////////////////////////////////
// plTestFrameworkResult public functions
////////////////////////////////////////////////////////////////////////

void plTestFrameworkResult::plTestResult::Reset()
{
  m_Result.Reset();
  const plUInt32 uiSubTestCount = (plUInt32)m_SubTests.size();
  for (plUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
  {
    m_SubTests[uiSubTest].m_Result.Reset();
  }
}



PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestResults);
