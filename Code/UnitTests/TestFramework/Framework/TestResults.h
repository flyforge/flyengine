#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/TestFrameworkDLL.h>
#include <deque>
#include <string>
#include <vector>

struct plTestOutput
{
  /// \brief Defines the type of output message for plTestOutputMessage.
  enum Enum
  {
    InvalidType = -1,
    StartOutput = 0,
    BeginBlock,
    EndBlock,
    ImportantInfo,
    Details,
    Success,
    Message,
    Warning,
    Error,
    ImageDiffFile,
    Duration,
    FinalResult,
    AllOutputTypes
  };

  static const char* const s_Names[];
  static const char* ToString(Enum type);
  static Enum FromString(const char* szName);
};

/// \brief A message of type plTestOutput::Enum, stored in plResult.
struct plTestErrorMessage
{
  plTestErrorMessage()

    = default;

  std::string m_sError;
  std::string m_sBlock;
  std::string m_sFile;
  plInt32 m_iLine = -1;
  std::string m_sFunction;
  std::string m_sMessage;
};

/// \brief A message of type plTestOutput::Enum, stored in plResult.
struct plTestOutputMessage
{
  plTestOutputMessage()

    = default;

  plTestOutput::Enum m_Type = plTestOutput::ImportantInfo;
  std::string m_sMessage;
  plInt32 m_iErrorIndex = -1;
};

struct plTestResultQuery
{
  /// \brief Defines what information should be accumulated over the sub-tests in plTestEntry::GetSubTestCount.
  enum Enum
  {
    Count,
    Executed,
    Success,
    Errors,
  };
};

/// \brief Stores the results of a test run. Used by both plTestEntry and plSubTestEntry.
struct plTestResultData
{
  plTestResultData()

    = default;
  void Reset();
  void AddOutput(plInt32 iOutputIndex);

  std::string m_sName;
  bool m_bExecuted = false;     ///< Whether the test was executed. If false, the test was either deactivated or the test process crashed before
                                ///< executing it.
  bool m_bSuccess = false;      ///< Whether the test succeeded or not.
  int m_iTestAsserts = 0;       ///< Asserts that were checked. For tests this includes the count of all of their sub-tests as well.
  double m_fTestDuration = 0.0; ///< Duration of the test/sub-test. For tests, this includes the duration of all their sub-tests as well.
  plInt32 m_iFirstOutput = -1;  ///< First output message. For tests, this range includes all messages of their sub-tests as well.
  plInt32 m_iLastOutput = -1;   ///< Last output message. For tests, this range includes all messages of their sub-tests as well.
};

struct plTestConfiguration
{
  plTestConfiguration();

  plUInt64 m_uiInstalledMainMemory = 0;
  plUInt32 m_uiMemoryPageSize = 0;
  plUInt32 m_uiCPUCoreCount = 0;
  bool m_b64BitOS = false;
  bool m_b64BitApplication = false;
  std::string m_sPlatformName;
  std::string m_sBuildConfiguration; ///< Debug, Release, etc
  plInt64 m_iDateTime = 0;           ///< in seconds since Linux epoch
  plInt32 m_iRCSRevision = -1;
  std::string m_sHostName;
};

class plTestFrameworkResult
{
public:
  plTestFrameworkResult() = default;

  // Manage tests
  void Clear();
  void SetupTests(const std::deque<plTestEntry>& tests, const plTestConfiguration& config);
  void Reset();
  bool WriteJsonToFile(const char* szFileName) const;

  // Result access
  plUInt32 GetTestCount(plTestResultQuery::Enum countQuery = plTestResultQuery::Count) const;
  plUInt32 GetSubTestCount(plUInt32 uiTestIndex, plTestResultQuery::Enum countQuery = plTestResultQuery::Count) const;
  plInt32 GetTestIndexByName(const char* szTestName) const;
  plInt32 GetSubTestIndexByName(plUInt32 uiTestIndex, const char* szSubTestName) const;
  double GetTotalTestDuration() const;
  const plTestResultData& GetTestResultData(plUInt32 uiTestIndex, plInt32 iSubTestIndex) const;

  // Test output
  void TestOutput(plUInt32 uiTestIndex, plInt32 iSubTestIndex, plTestOutput::Enum type, const char* szMsg);
  void TestError(plUInt32 uiTestIndex, plInt32 iSubTestIndex, const char* szError, const char* szBlock, const char* szFile, plInt32 iLine,
    const char* szFunction, const char* szMsg);
  void TestResult(plUInt32 uiTestIndex, plInt32 iSubTestIndex, bool bSuccess, double fDuration);
  void AddAsserts(plUInt32 uiTestIndex, plInt32 iSubTestIndex, int iCount);

  // Messages / Errors
  plUInt32 GetOutputMessageCount(plInt32 iTestIndex = -1, plInt32 iSubTestIndex = -1, plTestOutput::Enum type = plTestOutput::AllOutputTypes) const;
  const plTestOutputMessage* GetOutputMessage(plUInt32 uiOutputMessageIdx) const;

  plUInt32 GetErrorMessageCount(plInt32 iTestIndex = -1, plInt32 iSubTestIndex = -1) const;
  const plTestErrorMessage* GetErrorMessage(plUInt32 uiErrorMessageIdx) const;

private:
  struct plSubTestResult
  {
    plSubTestResult() = default;
    plSubTestResult(const char* szName) { m_Result.m_sName = szName; }

    plTestResultData m_Result;
  };

  struct plTestResult
  {
    plTestResult() = default;
    plTestResult(const char* szName) { m_Result.m_sName = szName; }

    void Reset();

    plTestResultData m_Result;
    std::deque<plSubTestResult> m_SubTests;
  };

private:
  plTestConfiguration m_Config;
  std::deque<plTestResult> m_Tests;
  std::deque<plTestErrorMessage> m_Errors;
  std::deque<plTestOutputMessage> m_TestOutput;
};
