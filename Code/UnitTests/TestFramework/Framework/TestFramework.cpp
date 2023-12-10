#include <TestFramework/TestFrameworkPCH.h>

#include <Texture/Image/Formats/ImageFileFormat.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/System/Process.h>
#include <Foundation/System/StackTracer.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <TestFramework/Utilities/TestOrder.h>

#include <cstdlib>
#include <stdexcept>
#include <stdlib.h>

#ifdef PLASMA_TESTFRAMEWORK_USE_FILESERVE
#  include <FileservePlugin/Client/FileserveClient.h>
#  include <FileservePlugin/Client/FileserveDataDir.h>
#  include <FileservePlugin/FileservePluginDLL.h>
#endif

plTestFramework* plTestFramework::s_pInstance = nullptr;

const char* plTestFramework::s_szTestBlockName = "";
int plTestFramework::s_iAssertCounter = 0;
bool plTestFramework::s_bCallstackOnAssert = false;
plLog::TimestampMode plTestFramework::s_LogTimestampMode = plLog::TimestampMode::None;

plCommandLineOptionPath opt_OrderFile("_TestFramework", "-order", "Path to a file that defines which tests to run.", "");
plCommandLineOptionPath opt_SettingsFile("_TestFramework", "-settings", "Path to a file containing the test settings.", "");
plCommandLineOptionBool opt_Run("_TestFramework", "-run", "Makes the tests execute right away.", false);
plCommandLineOptionBool opt_Close("_TestFramework", "-close", "Makes the application close automatically after the tests are finished.", false);
plCommandLineOptionBool opt_NoGui("_TestFramework", "-noGui", "Never show a GUI.", false);
plCommandLineOptionBool opt_HTML("_TestFramework", "-html", "Open summary HTML on error.", false);
plCommandLineOptionBool opt_Console("_TestFramework", "-console", "Keep the console open.", false);
plCommandLineOptionBool opt_Timestamps("_TestFramework", "-timestamps", "Show timestamps in logs.", false);
plCommandLineOptionBool opt_MsgBox("_TestFramework", "-msgbox", "Show message box after tests.", false);
plCommandLineOptionBool opt_DisableSuccessful("_TestFramework", "-disableSuccessful", "Disable tests that ran successfully.", false);
plCommandLineOptionBool opt_EnableAllTests("_TestFramework", "-all", "Enable all tests.", false);
plCommandLineOptionBool opt_NoSave("_TestFramework", "-noSave", "Disables saving of any state.", false);
plCommandLineOptionInt opt_Revision("_TestFramework", "-rev", "Revision number to pass through to JSON output.", -1);
plCommandLineOptionInt opt_Passes("_TestFramework", "-passes", "Number of passes to execute.", 1);
plCommandLineOptionInt opt_Assert("_TestFramework", "-assert", "Whether to assert when a test fails.", (int)AssertOnTestFail::AssertIfDebuggerAttached);
plCommandLineOptionString opt_Filter("_TestFramework", "-filter", "Filter to execute only certain tests.", "");
plCommandLineOptionPath opt_Json("_TestFramework", "-json", "JSON file to write.", "");
plCommandLineOptionPath opt_OutputDir("_TestFramework", "-outputDir", "Output directory", "");

constexpr int s_iMaxErrorMessageLength = 512;

static bool TestAssertHandler(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  if (plTestFramework::s_bCallstackOnAssert)
  {
    void* pBuffer[64];
    plArrayPtr<void*> tempTrace(pBuffer);
    const plUInt32 uiNumTraces = plStackTracer::GetStackTrace(tempTrace, nullptr);
    plStackTracer::ResolveStackTrace(tempTrace.GetSubArray(0, uiNumTraces), &plLog::Print);
  }

  plTestFramework::Error(szExpression, szSourceFile, (plInt32)uiLine, szFunction, szAssertMsg);

  // if a debugger is attached, one typically always wants to know about asserts
  if (plSystemInformation::IsDebuggerAttached())
    return true;

  plTestFramework::GetInstance()->AbortTests();

  return plTestFramework::GetAssertOnTestFail();
}

////////////////////////////////////////////////////////////////////////
// plTestFramework public functions
////////////////////////////////////////////////////////////////////////

plTestFramework::plTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int iArgc, const char** pArgv)
  : m_sTestName(szTestName)
  , m_sAbsTestOutputDir(szAbsTestOutputDir)
  , m_sRelTestDataDir(szRelTestDataDir)
{
  s_pInstance = this;

  plCommandLineUtils::GetGlobalInstance()->SetCommandLine(iArgc, pArgv, plCommandLineUtils::PreferOsArgs);

  GetTestSettingsFromCommandLine(*plCommandLineUtils::GetGlobalInstance());
}

plTestFramework::~plTestFramework()
{
  if (m_bIsInitialized)
    DeInitialize();
  s_pInstance = nullptr;
}

void plTestFramework::Initialize()
{
  {
    plStringBuilder cmdHelp;
    if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested, "_TestFramework;cvar"))
    {
      // make sure the console stays open
      plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-console");
      plCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("true");

      plLog::Print(cmdHelp);
    }
  }

  if (m_Settings.m_bNoGUI)
  {
    // if the UI is run with GUI disabled, set the environment variable PLASMA_SILENT_ASSERTS
    // to make sure that no child process that the tests launch shows an assert dialog in case of a crash
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
    // Not supported
#else
    if (plEnvironmentVariableUtils::SetValueInt("PLASMA_SILENT_ASSERTS", 1).Failed())
    {
      plLog::Print("Failed to set 'PLASMA_SILENT_ASSERTS' environment variable!");
    }
#endif
  }

  if (m_Settings.m_bShowTimestampsInLog)
  {
    plTestFramework::s_LogTimestampMode = plLog::TimestampMode::TimeOnly;
    plLogWriter::Console::SetTimestampMode(plLog::TimestampMode::TimeOnly);
  }

  // Don't do this, it will spam the log with sub-system messages
  // plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  // plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  plStartup::AddApplicationTag("testframework");
  plStartup::StartupCoreSystems();
  PLASMA_SCOPE_EXIT(plStartup::ShutdownCoreSystems());

  // if tests need to write data back through Fileserve (e.g. image comparison results), they can do that through a data dir mounted with
  // this path
  plFileSystem::SetSpecialDirectory("pltest", plTestFramework::GetInstance()->GetAbsOutputPath());

  // Setting pl assert handler
  m_PreviousAssertHandler = plGetAssertHandler();
  plSetAssertHandler(TestAssertHandler);

  CreateOutputFolder();

  plCommandLineUtils& cmd = *plCommandLineUtils::GetGlobalInstance();
  // figure out which tests exist
  GatherAllTests();

  if (!m_Settings.m_bNoGUI || opt_OrderFile.IsOptionSpecified(nullptr, &cmd))
  {
    // load the test order from file, if that file does not exist, the array is not modified.
    LoadTestOrder();
  }
  ApplyTestOrderFromCommandLine(cmd);

  if (!m_Settings.m_bNoGUI || opt_SettingsFile.IsOptionSpecified(nullptr, &cmd))
  {
    // Load the test settings from file, if that file does not exist, the settings are not modified.
    LoadTestSettings();
    // Overwrite loaded test settings with command line
    GetTestSettingsFromCommandLine(cmd);
  }

  // save the current order back to the same file
  AutoSaveTestOrder();

  m_bIsInitialized = true;

  plFileSystem::DetectSdkRootDirectory().IgnoreResult();
}

void plTestFramework::DeInitialize()
{
  m_bIsInitialized = false;

  plSetAssertHandler(m_PreviousAssertHandler);
  m_PreviousAssertHandler = nullptr;
}

const char* plTestFramework::GetTestName() const
{
  return m_sTestName.c_str();
}

const char* plTestFramework::GetAbsOutputPath() const
{
  return m_sAbsTestOutputDir.c_str();
}


const char* plTestFramework::GetRelTestDataPath() const
{
  return m_sRelTestDataDir.c_str();
}

const char* plTestFramework::GetAbsTestOrderFilePath() const
{
  return m_sAbsTestOrderFilePath.c_str();
}

const char* plTestFramework::GetAbsTestSettingsFilePath() const
{
  return m_sAbsTestSettingsFilePath.c_str();
}

void plTestFramework::RegisterOutputHandler(OutputHandler handler)
{
  // do not register a handler twice
  for (plUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    if (m_OutputHandlers[i] == handler)
      return;
  }

  m_OutputHandlers.push_back(handler);
}


void plTestFramework::SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider)
{
  m_ImageDiffExtraInfoCallback = provider;
}

bool plTestFramework::GetAssertOnTestFail()
{
  switch (s_pInstance->m_Settings.m_AssertOnTestFail)
  {
    case AssertOnTestFail::DoNotAssert:
      return false;
    case AssertOnTestFail::AssertIfDebuggerAttached:
      return plSystemInformation::IsDebuggerAttached();
    case AssertOnTestFail::AlwaysAssert:
      return true;
  }
  return false;
}

void plTestFramework::GatherAllTests()
{
  m_TestEntries.clear();

  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;
  m_iExecutingTest = -1;
  m_iExecutingSubTest = -1;
  m_bSubTestInitialized = false;

  // first let all simple tests register themselves
  {
    plRegisterSimpleTestHelper* pHelper = plRegisterSimpleTestHelper::GetFirstInstance();

    while (pHelper)
    {
      pHelper->RegisterTest();

      pHelper = pHelper->GetNextInstance();
    }
  }

  plTestConfiguration config;
  plTestBaseClass* pTestClass = plTestBaseClass::GetFirstInstance();

  while (pTestClass)
  {
    pTestClass->ClearSubTests();
    pTestClass->SetupSubTests();
    pTestClass->UpdateConfiguration(config);

    plTestEntry e;
    e.m_pTest = pTestClass;
    e.m_szTestName = pTestClass->GetTestName();
    e.m_sNotAvailableReason = pTestClass->IsTestAvailable();

    for (plUInt32 i = 0; i < pTestClass->m_Entries.size(); ++i)
    {
      plSubTestEntry st;
      st.m_szSubTestName = pTestClass->m_Entries[i].m_szName;
      st.m_iSubTestIdentifier = pTestClass->m_Entries[i].m_iIdentifier;

      e.m_SubTests.push_back(st);
    }

    m_TestEntries.push_back(e);

    pTestClass = pTestClass->GetNextInstance();
  }
  ::SortTestsAlphabetically(m_TestEntries);

  m_Result.SetupTests(m_TestEntries, config);
}

void plTestFramework::GetTestSettingsFromCommandLine(const plCommandLineUtils& cmd)
{
  // use a local instance of plCommandLineUtils as global instance is not guaranteed to have been set up
  // for all call sites of this method.

  m_Settings.m_bRunTests = opt_Run.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_bCloseOnSuccess = opt_Close.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_bNoGUI = opt_NoGui.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  if (opt_Assert.IsOptionSpecified(nullptr, &cmd))
  {
    const int assertOnTestFailure = opt_Assert.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
    switch (assertOnTestFailure)
    {
      case 0:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::DoNotAssert;
        break;
      case 1:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::AssertIfDebuggerAttached;
        break;
      case 2:
        m_Settings.m_AssertOnTestFail = AssertOnTestFail::AlwaysAssert;
        break;
    }
  }

  plStringBuilder tmp;

  opt_HTML.SetDefaultValue(m_Settings.m_bOpenHtmlOutputOnError);
  m_Settings.m_bOpenHtmlOutputOnError = opt_HTML.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_Console.SetDefaultValue(m_Settings.m_bKeepConsoleOpen);
  m_Settings.m_bKeepConsoleOpen = opt_Console.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_Timestamps.SetDefaultValue(m_Settings.m_bShowTimestampsInLog);
  m_Settings.m_bShowTimestampsInLog = opt_Timestamps.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_MsgBox.SetDefaultValue(m_Settings.m_bShowMessageBox);
  m_Settings.m_bShowMessageBox = opt_MsgBox.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  opt_DisableSuccessful.SetDefaultValue(m_Settings.m_bAutoDisableSuccessfulTests);
  m_Settings.m_bAutoDisableSuccessfulTests = opt_DisableSuccessful.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  m_Settings.m_iRevision = opt_Revision.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_bEnableAllTests = opt_EnableAllTests.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  m_Settings.m_uiFullPasses = static_cast<plUInt8>(opt_Passes.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd));
  m_Settings.m_sTestFilter = opt_Filter.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd).GetData(tmp);

  if (opt_Json.IsOptionSpecified(nullptr, &cmd))
  {
    m_Settings.m_sJsonOutput = opt_Json.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  }

  if (opt_OutputDir.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestOutputDir = opt_OutputDir.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);
  }

  bool bNoAutoSave = false;
  if (opt_OrderFile.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestOrderFilePath = opt_OrderFile.GetOptionValue(plCommandLineOption::LogMode::Always);
    // If a custom order file was provided, default to -nosave as to not overwrite that file with additional
    // parameters from command line. Use "-nosave false" to explicitly enable auto save in this case.
    bNoAutoSave = true;
  }
  else
  {
    m_sAbsTestOrderFilePath = m_sAbsTestOutputDir + std::string("/TestOrder.txt");
  }

  if (opt_SettingsFile.IsOptionSpecified(nullptr, &cmd))
  {
    m_sAbsTestSettingsFilePath = opt_SettingsFile.GetOptionValue(plCommandLineOption::LogMode::Always);
    // If a custom settings file was provided, default to -nosave as to not overwrite that file with additional
    // parameters from command line. Use "-nosave false" to explicitly enable auto save in this case.
    bNoAutoSave = true;
  }
  else
  {
    m_sAbsTestSettingsFilePath = m_sAbsTestOutputDir + std::string("/TestSettings.txt");
  }
  opt_NoSave.SetDefaultValue(bNoAutoSave);
  m_Settings.m_bNoAutomaticSaving = opt_NoSave.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified, &cmd);

  m_uiPassesLeft = m_Settings.m_uiFullPasses;
}

void plTestFramework::LoadTestOrder()
{
  ::LoadTestOrder(m_sAbsTestOrderFilePath.c_str(), m_TestEntries);
}

void plTestFramework::ApplyTestOrderFromCommandLine(const plCommandLineUtils& cmd)
{
  if (m_Settings.m_bEnableAllTests)
    SetAllTestsEnabledStatus(true);
  if (!m_Settings.m_sTestFilter.empty())
  {
    const plUInt32 uiTestCount = GetTestCount();
    for (plUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
    {
      const bool bEnable = plStringUtils::FindSubString_NoCase(m_TestEntries[uiTestIdx].m_szTestName, m_Settings.m_sTestFilter.c_str()) != nullptr;
      m_TestEntries[uiTestIdx].m_bEnableTest = bEnable;
      const plUInt32 uiSubTestCount = (plUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
      for (plUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
      {
        m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = bEnable;
      }
    }
  }
}

void plTestFramework::LoadTestSettings()
{
  ::LoadTestSettings(m_sAbsTestSettingsFilePath.c_str(), m_Settings);
}

void plTestFramework::CreateOutputFolder()
{
  plOSFile::CreateDirectoryStructure(m_sAbsTestOutputDir.c_str()).IgnoreResult();

  PLASMA_ASSERT_RELEASE(plOSFile::ExistsDirectory(m_sAbsTestOutputDir.c_str()), "Failed to create output directory '{0}'", m_sAbsTestOutputDir.c_str());
}

void plTestFramework::UpdateReferenceImages()
{
  plStringBuilder sDir;
  if (plFileSystem::ResolveSpecialDirectory(">sdk", sDir).Failed())
    return;

  sDir.AppendPath(GetRelTestDataPath());

  const plStringBuilder sNewFiles(m_sAbsTestOutputDir.c_str(), "/Images_Result");
  const plStringBuilder sRefFiles(sDir, "/", m_sImageReferenceFolderName.c_str());

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS) && PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)


#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  plStringBuilder sOptiPng = plFileSystem::GetSdkRootDirectory();
  sOptiPng.AppendPath("Data/Tools/Precompiled/optipng/optipng.exe");

  if (plOSFile::ExistsFile(sOptiPng))
  {
    plStringBuilder sPath;

    plFileSystemIterator it;
    it.StartSearch(sNewFiles, plFileSystemIteratorFlags::ReportFiles);
    for (; it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sPath);

      plProcessOptions opt;
      opt.m_sProcess = sOptiPng;
      opt.m_Arguments.PushBack(sPath);
      plProcess::Execute(opt).IgnoreResult();
    }
  }

#  endif

  // if some target files already exist somewhere (ie. custom folders for the tests)
  // overwrite the existing files in their location
  {
    plHybridArray<plString, 32> targetFolders;
    plStringBuilder sFullPath, sTargetPath;

    {
      plFileSystemIterator it;
      it.StartSearch(sDir, plFileSystemIteratorFlags::ReportFoldersRecursive);
      for (; it.IsValid(); it.Next())
      {
        if (it.GetStats().m_sName == m_sImageReferenceFolderName.c_str())
        {
          it.GetStats().GetFullPath(sFullPath);

          targetFolders.PushBack(sFullPath);
        }
      }
    }

    plFileSystemIterator it;
    it.StartSearch(sNewFiles, plFileSystemIteratorFlags::ReportFiles);
    for (; it.IsValid(); it.Next())
    {
      it.GetStats().GetFullPath(sFullPath);

      for (plUInt32 i = 0; i < targetFolders.GetCount(); ++i)
      {
        sTargetPath = targetFolders[i];
        sTargetPath.AppendPath(it.GetStats().m_sName);

        if (plOSFile::ExistsFile(sTargetPath))
        {
          plOSFile::DeleteFile(sTargetPath).IgnoreResult();
          plOSFile::MoveFileOrDirectory(sFullPath, sTargetPath).IgnoreResult();
          break;
        }
      }
    }
  }

  // copy the remaining files to the default directory
  plOSFile::CopyFolder(sNewFiles, sRefFiles).IgnoreResult();
  plOSFile::DeleteFolder(sNewFiles).IgnoreResult();
#endif
}

void plTestFramework::AutoSaveTestOrder()
{
  if (m_Settings.m_bNoAutomaticSaving)
    return;

  SaveTestOrder(m_sAbsTestOrderFilePath.c_str());
  SaveTestSettings(m_sAbsTestSettingsFilePath.c_str());
}

void plTestFramework::SaveTestOrder(const char* const szFilePath)
{
  ::SaveTestOrder(szFilePath, m_TestEntries);
}

void plTestFramework::SaveTestSettings(const char* const szFilePath)
{
  ::SaveTestSettings(szFilePath, m_Settings);
}

void plTestFramework::SetAllTestsEnabledStatus(bool bEnable)
{
  const plUInt32 uiTestCount = GetTestCount();
  for (plUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    m_TestEntries[uiTestIdx].m_bEnableTest = bEnable;
    const plUInt32 uiSubTestCount = (plUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (plUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = bEnable;
    }
  }
}

void plTestFramework::SetAllFailedTestsEnabledStatus()
{
  const auto& LastResult = GetTestResult();

  const plUInt32 uiTestCount = GetTestCount();
  for (plUInt32 uiTestIdx = 0; uiTestIdx < uiTestCount; ++uiTestIdx)
  {
    const auto& TestRes = LastResult.GetTestResultData(uiTestIdx, -1);
    m_TestEntries[uiTestIdx].m_bEnableTest = TestRes.m_bExecuted && !TestRes.m_bSuccess;

    const plUInt32 uiSubTestCount = (plUInt32)m_TestEntries[uiTestIdx].m_SubTests.size();
    for (plUInt32 uiSubTest = 0; uiSubTest < uiSubTestCount; ++uiSubTest)
    {
      const auto& SubTestRes = LastResult.GetTestResultData(uiTestIdx, uiSubTest);
      m_TestEntries[uiTestIdx].m_SubTests[uiSubTest].m_bEnableTest = SubTestRes.m_bExecuted && !SubTestRes.m_bSuccess;
    }
  }
}

void plTestFramework::SetTestTimeout(plUInt32 uiTestTimeoutMS)
{
  {
    std::scoped_lock<std::mutex> lock(m_TimeoutLock);
    m_uiTimeoutMS = uiTestTimeoutMS;
  }
  UpdateTestTimeout();
}

plUInt32 plTestFramework::GetTestTimeout() const
{
  return m_uiTimeoutMS;
}

void plTestFramework::TimeoutThread()
{
  std::unique_lock<std::mutex> lock(m_TimeoutLock);
  while (m_bUseTimeout)
  {
    if (m_uiTimeoutMS == 0)
    {
      // If no timeout is set, we simply put the thread to sleep.
      m_TimeoutCV.wait(lock, [this] { return !m_bUseTimeout; });
    }
    // We want to be notified when we reach the timeout and not when we are spuriously woken up.
    // Thus we continue waiting via the predicate if we are still using a timeout until we are either
    // woken up via the CV or reach the timeout.
    else if (!m_TimeoutCV.wait_for(lock, std::chrono::milliseconds(m_uiTimeoutMS), [this] { return !m_bUseTimeout || m_bArm; }))
    {
      if (plSystemInformation::IsDebuggerAttached())
      {
        // Should we attach a debugger mid run and reach the timeout we obviously do not want to terminate.
        continue;
      }

      // CV was not signaled until the timeout was reached.
      plTestFramework::Output(plTestOutput::Error, "Timeout reached, terminating app.");
      // The top level exception handler takes care of all the shutdown logic already (app specific logic, crash dump, callstack etc)
      // which we do not want to duplicate here so we simply throw an unhandled exception.
      throw std::runtime_error("Timeout reached, terminating app.");
    }
    m_bArm = false;
  }
}


void plTestFramework::UpdateTestTimeout()
{
  {
    std::scoped_lock<std::mutex> lock(m_TimeoutLock);
    if (!m_bUseTimeout)
    {
      return;
    }
    m_bArm = true;
  }
  m_TimeoutCV.notify_one();
}

void plTestFramework::ResetTests()
{
  m_iErrorCount = 0;
  m_iTestsFailed = 0;
  m_iTestsPassed = 0;
  m_iExecutingTest = -1;
  m_iExecutingSubTest = -1;
  m_bSubTestInitialized = false;
  m_bAbortTests = false;

  m_Result.Reset();
}

plTestAppRun plTestFramework::RunTestExecutionLoop()
{
  if (!m_bIsInitialized)
  {
    Initialize();

#ifdef PLASMA_TESTFRAMEWORK_USE_FILESERVE
    if (plFileserveClient::GetSingleton() == nullptr)
    {
      PLASMA_DEFAULT_NEW(plFileserveClient);

      if (plFileserveClient::GetSingleton()->SearchForServerAddress().Failed())
      {
        plFileserveClient::GetSingleton()->WaitForServerInfo().IgnoreResult();
      }
    }

    if (plFileserveClient::GetSingleton()->EnsureConnected(plTime::Seconds(-30)).Failed())
    {
      Error("Failed to establish a Fileserve connection", "", 0, "plTestFramework::RunTestExecutionLoop", "");
      return plTestAppRun::Quit;
    }
#endif
  }

#ifdef PLASMA_TESTFRAMEWORK_USE_FILESERVE
  plFileserveClient::GetSingleton()->UpdateClient();
#endif


  if (m_iExecutingTest < 0)
  {
    StartTests();
    m_iExecutingTest = 0;
    PLASMA_ASSERT_DEV(m_iExecutingSubTest == -1, "Invalid test framework state");
    PLASMA_ASSERT_DEV(!m_bSubTestInitialized, "Invalid test framework state");
  }

  ExecuteNextTest();

  if (m_iExecutingTest >= (plInt32)m_TestEntries.size())
  {
    EndTests();

    if (m_uiPassesLeft > 1 && !m_bAbortTests)
    {
      --m_uiPassesLeft;

      m_iExecutingTest = -1;
      m_iExecutingSubTest = -1;

      return plTestAppRun::Continue;
    }

#ifdef PLASMA_TESTFRAMEWORK_USE_FILESERVE
    if (plFileserveClient* pClient = plFileserveClient::GetSingleton())
    {
      // shutdown the fileserve client
      PLASMA_DEFAULT_DELETE(pClient);
    }
#endif

    return plTestAppRun::Quit;
  }

  return plTestAppRun::Continue;
}

void plTestFramework::StartTests()
{
  ResetTests();
  m_bTestsRunning = true;
  plTestFramework::Output(plTestOutput::StartOutput, "");

  // Start timeout thread.
  std::scoped_lock lock(m_TimeoutLock);
  m_bUseTimeout = true;
  m_bArm = false;
  m_TimeoutThread = std::thread(&plTestFramework::TimeoutThread, this);
}

// Redirects engine warnings / errors to test-framework output
static void LogWriter(const plLoggingEventData& e)
{
  const plStringBuilder sText = e.m_sText;

  switch (e.m_EventType)
  {
    case plLogMsgType::ErrorMsg:
      plTestFramework::Output(plTestOutput::Error, "plLog Error: %s", sText.GetData());
      break;
    case plLogMsgType::SeriousWarningMsg:
      plTestFramework::Output(plTestOutput::Error, "plLog Serious Warning: %s", sText.GetData());
      break;
    case plLogMsgType::WarningMsg:
      plTestFramework::Output(plTestOutput::Warning, "plLog Warning: %s", sText.GetData());
      break;
    case plLogMsgType::InfoMsg:
    case plLogMsgType::DevMsg:
    case plLogMsgType::DebugMsg:
    {
      if (e.m_sTag.IsEqual_NoCase("test"))
        plTestFramework::Output(plTestOutput::Details, sText.GetData());
    }
    break;

    default:
      return;
  }
}

void plTestFramework::ExecuteNextTest()
{
  PLASMA_ASSERT_DEV(m_iExecutingTest >= 0, "Invalid current test.");

  if (m_iExecutingTest == (plInt32)GetTestCount())
    return;

  if (!m_TestEntries[m_iExecutingTest].m_bEnableTest)
  {
    // next time run the next test and start with the first subtest
    m_iExecutingTest++;
    m_iExecutingSubTest = -1;
    return;
  }

  plTestEntry& TestEntry = m_TestEntries[m_iExecutingTest];
  plTestBaseClass* pTestClass = m_TestEntries[m_iExecutingTest].m_pTest;

  // Execute test
  {
    if (m_iExecutingSubTest == -1) // no subtest has run yet, so initialize the test first
    {
      if (m_bAbortTests)
      {
        m_iExecutingTest = (plInt32)m_TestEntries.size(); // skip to the end of all tests
        m_iExecutingSubTest = -1;
        return;
      }

      m_iExecutingSubTest = 0;
      m_fTotalTestDuration = 0.0;

      // Reset assert counter. This variable is used to reduce the overhead of counting millions of asserts.
      s_iAssertCounter = 0;
      m_iCurrentTestIndex = m_iExecutingTest;
      // Log writer translates engine warnings / errors into test framework error messages.
      plGlobalLog::AddLogWriter(LogWriter);

      m_iErrorCountBeforeTest = GetTotalErrorCount();

      plTestFramework::Output(plTestOutput::BeginBlock, "Executing Test: '%s'", TestEntry.m_szTestName);

      // *** Test Initialization ***
      if (TestEntry.m_sNotAvailableReason.empty())
      {
        UpdateTestTimeout();
        if (pTestClass->DoTestInitialization().Failed())
        {
          m_iExecutingSubTest = (plInt32)TestEntry.m_SubTests.size(); // make sure all sub-tests are skipped
        }
      }
      else
      {
        plTestFramework::Output(plTestOutput::ImportantInfo, "Test not available: %s", TestEntry.m_sNotAvailableReason.c_str());
        m_iExecutingSubTest = (plInt32)TestEntry.m_SubTests.size(); // make sure all sub-tests are skipped
      }
    }

    if (m_iExecutingSubTest < (plInt32)TestEntry.m_SubTests.size())
    {
      plSubTestEntry& subTest = TestEntry.m_SubTests[m_iExecutingSubTest];
      plInt32 iSubTestIdentifier = subTest.m_iSubTestIdentifier;

      if (!subTest.m_bEnableTest)
      {
        ++m_iExecutingSubTest;
        return;
      }

      if (!m_bSubTestInitialized)
      {
        if (m_bAbortTests)
        {
          // tests shall be aborted, so do not start a new one

          m_iExecutingTest = (plInt32)m_TestEntries.size(); // skip to the end of all tests
          m_iExecutingSubTest = -1;
          return;
        }

        m_fTotalSubTestDuration = 0.0;
        m_uiSubTestInvocationCount = 0;

        // First flush of assert counter, these are all asserts during test init.
        FlushAsserts();
        m_iCurrentSubTestIndex = m_iExecutingSubTest;
        plTestFramework::Output(plTestOutput::BeginBlock, "Executing Sub-Test: '%s'", subTest.m_szSubTestName);

        // *** Sub-Test Initialization ***
        UpdateTestTimeout();
        m_bSubTestInitialized = pTestClass->DoSubTestInitialization(iSubTestIdentifier).Succeeded();
      }

      plTestAppRun subTestResult = plTestAppRun::Quit;

      if (m_bSubTestInitialized)
      {
        // *** Run Sub-Test ***
        double fDuration = 0.0;

        // start with 1
        ++m_uiSubTestInvocationCount;

        UpdateTestTimeout();
        subTestResult = pTestClass->DoSubTestRun(iSubTestIdentifier, fDuration, m_uiSubTestInvocationCount);
        s_szTestBlockName = "";

        if (m_bImageComparisonScheduled)
        {
          PLASMA_TEST_IMAGE(m_uiComparisonImageNumber, m_uiMaxImageComparisonError);
          m_bImageComparisonScheduled = false;
        }


        if (m_bDepthImageComparisonScheduled)
        {
          PLASMA_TEST_DEPTH_IMAGE(m_uiComparisonDepthImageNumber, m_uiMaxDepthImageComparisonError);
          m_bDepthImageComparisonScheduled = false;
        }

        // I guess we can require that tests are written in a way that they can be interrupted
        if (m_bAbortTests)
          subTestResult = plTestAppRun::Quit;

        m_fTotalSubTestDuration += fDuration;
      }

      // this is executed when sub-test initialization failed or the sub-test reached its end
      if (subTestResult == plTestAppRun::Quit)
      {
        // *** Sub-Test De-Initialization ***
        UpdateTestTimeout();
        pTestClass->DoSubTestDeInitialization(iSubTestIdentifier);

        bool bSubTestSuccess = m_bSubTestInitialized && (m_Result.GetErrorMessageCount(m_iExecutingTest, m_iExecutingSubTest) == 0);
        plTestFramework::TestResult(m_iExecutingSubTest, bSubTestSuccess, m_fTotalSubTestDuration);

        m_fTotalTestDuration += m_fTotalSubTestDuration;

        // advance to the next (sub) test
        m_bSubTestInitialized = false;
        ++m_iExecutingSubTest;

        // Second flush of assert counter, these are all asserts for the current subtest.
        FlushAsserts();
        plTestFramework::Output(plTestOutput::EndBlock, "");
        m_iCurrentSubTestIndex = -1;
      }
    }

    if (m_bAbortTests || m_iExecutingSubTest >= (plInt32)TestEntry.m_SubTests.size())
    {
      // *** Test De-Initialization ***
      if (TestEntry.m_sNotAvailableReason.empty())
      {
        // We only call DoTestInitialization under this condition so DoTestDeInitialization must be guarded by the same.
        UpdateTestTimeout();
        pTestClass->DoTestDeInitialization();
      }
      // Third and last flush of assert counter, these are all asserts for the test de-init.
      FlushAsserts();

      plGlobalLog::RemoveLogWriter(LogWriter);

      bool bTestSuccess = m_iErrorCountBeforeTest == GetTotalErrorCount();
      plTestFramework::TestResult(-1, bTestSuccess, m_fTotalTestDuration);
      plTestFramework::Output(plTestOutput::EndBlock, "");
      m_iCurrentTestIndex = -1;

      // advance to the next test
      m_iExecutingTest++;
      m_iExecutingSubTest = -1;
    }
  }
}

void plTestFramework::EndTests()
{
  m_bTestsRunning = false;
  if (GetTestsFailedCount() == 0)
    plTestFramework::Output(plTestOutput::FinalResult, "All tests passed.");
  else
    plTestFramework::Output(plTestOutput::FinalResult, "Tests failed: %i. Tests passed: %i", GetTestsFailedCount(), GetTestsPassedCount());

  if (!m_Settings.m_sJsonOutput.empty())
    m_Result.WriteJsonToFile(m_Settings.m_sJsonOutput.c_str());

  m_iExecutingTest = -1;
  m_iExecutingSubTest = -1;
  m_bAbortTests = false;

  // Stop timeout thread.
  {
    std::scoped_lock lock(m_TimeoutLock);
    m_bUseTimeout = false;
    m_TimeoutCV.notify_one();
  }
  m_TimeoutThread.join();
}

void plTestFramework::AbortTests()
{
  m_bAbortTests = true;
}

plUInt32 plTestFramework::GetTestCount() const
{
  return (plUInt32)m_TestEntries.size();
}

plUInt32 plTestFramework::GetTestEnabledCount() const
{
  plUInt32 uiEnabledCount = 0;
  const plUInt32 uiTests = GetTestCount();
  for (plUInt32 uiTest = 0; uiTest < uiTests; ++uiTest)
  {
    uiEnabledCount += m_TestEntries[uiTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

plUInt32 plTestFramework::GetSubTestEnabledCount(plUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return 0;

  plUInt32 uiEnabledCount = 0;
  const plUInt32 uiSubTests = (plUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  for (plUInt32 uiSubTest = 0; uiSubTest < uiSubTests; ++uiSubTest)
  {
    uiEnabledCount += m_TestEntries[uiTestIndex].m_SubTests[uiSubTest].m_bEnableTest ? 1 : 0;
  }
  return uiEnabledCount;
}

const std::string& plTestFramework::IsTestAvailable(plUInt32 uiTestIndex) const
{
  PLASMA_ASSERT_DEV(uiTestIndex < GetTestCount(), "Test index {0} is larger than number of tests {1}.", uiTestIndex, GetTestCount());
  return m_TestEntries[uiTestIndex].m_sNotAvailableReason;
}

bool plTestFramework::IsTestEnabled(plUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  return m_TestEntries[uiTestIndex].m_bEnableTest;
}

bool plTestFramework::IsSubTestEnabled(plUInt32 uiTestIndex, plUInt32 uiSubTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return false;

  const plUInt32 uiSubTests = (plUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return false;

  return m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest;
}

void plTestFramework::SetTestEnabled(plUInt32 uiTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  m_TestEntries[uiTestIndex].m_bEnableTest = bEnabled;
}

void plTestFramework::SetSubTestEnabled(plUInt32 uiTestIndex, plUInt32 uiSubTestIndex, bool bEnabled)
{
  if (uiTestIndex >= GetTestCount())
    return;

  const plUInt32 uiSubTests = (plUInt32)m_TestEntries[uiTestIndex].m_SubTests.size();
  if (uiSubTestIndex >= uiSubTests)
    return;

  m_TestEntries[uiTestIndex].m_SubTests[uiSubTestIndex].m_bEnableTest = bEnabled;
}

plTestEntry* plTestFramework::GetTest(plUInt32 uiTestIndex)
{
  if (uiTestIndex >= GetTestCount())
    return nullptr;

  return &m_TestEntries[uiTestIndex];
}

const plTestEntry* plTestFramework::GetTest(plUInt32 uiTestIndex) const
{
  if (uiTestIndex >= GetTestCount())
    return nullptr;

  return &m_TestEntries[uiTestIndex];
}

const plTestEntry* plTestFramework::GetCurrentTest() const
{
  return GetTest(GetCurrentTestIndex());
}

const plSubTestEntry* plTestFramework::GetCurrentSubTest() const
{
  if (auto pTest = GetCurrentTest())
  {
    if (m_iCurrentSubTestIndex >= (plInt32)pTest->m_SubTests.size())
      return nullptr;

    return &pTest->m_SubTests[m_iCurrentSubTestIndex];
  }

  return nullptr;
}

TestSettings plTestFramework::GetSettings() const
{
  return m_Settings;
}

void plTestFramework::SetSettings(const TestSettings& settings)
{
  m_Settings = settings;
}

plTestFrameworkResult& plTestFramework::GetTestResult()
{
  return m_Result;
}

plInt32 plTestFramework::GetTotalErrorCount() const
{
  return m_iErrorCount;
}

plInt32 plTestFramework::GetTestsPassedCount() const
{
  return m_iTestsPassed;
}

plInt32 plTestFramework::GetTestsFailedCount() const
{
  return m_iTestsFailed;
}

double plTestFramework::GetTotalTestDuration() const
{
  return m_Result.GetTotalTestDuration();
}

////////////////////////////////////////////////////////////////////////
// plTestFramework protected functions
////////////////////////////////////////////////////////////////////////

static bool g_bBlockOutput = false;

void plTestFramework::OutputImpl(plTestOutput::Enum Type, const char* szMsg)
{
  std::scoped_lock _(m_OutputMutex);

  if (Type == plTestOutput::Error)
  {
    m_iErrorCount++;
  }
  // pass the output to all the registered output handlers, which will then write it to the console, file, etc.
  for (plUInt32 i = 0; i < m_OutputHandlers.size(); ++i)
  {
    m_OutputHandlers[i](Type, szMsg);
  }

  if (g_bBlockOutput)
    return;

  m_Result.TestOutput(m_iCurrentTestIndex, m_iCurrentSubTestIndex, Type, szMsg);
}

void plTestFramework::ErrorImpl(const char* szError, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg)
{
  std::scoped_lock _(m_OutputMutex);

  m_Result.TestError(m_iCurrentTestIndex, m_iCurrentSubTestIndex, szError, plTestFramework::s_szTestBlockName, szFile, iLine, szFunction, szMsg);

  g_bBlockOutput = true;
  plTestFramework::Output(plTestOutput::Error, "%s", szError); // This will also increase the global error count.
  plTestFramework::Output(plTestOutput::BeginBlock, "");
  {
    if ((plTestFramework::s_szTestBlockName != nullptr) && (plTestFramework::s_szTestBlockName[0] != '\0'))
      plTestFramework::Output(plTestOutput::Message, "Block: '%s'", plTestFramework::s_szTestBlockName);

    plTestFramework::Output(plTestOutput::ImportantInfo, "File: %s", szFile);
    plTestFramework::Output(plTestOutput::ImportantInfo, "Line: %i", iLine);
    plTestFramework::Output(plTestOutput::ImportantInfo, "Function: %s", szFunction);

    if ((szMsg != nullptr) && (szMsg[0] != '\0'))
      plTestFramework::Output(plTestOutput::Message, "Error: %s", szMsg);
  }
  plTestFramework::Output(plTestOutput::EndBlock, "");
  g_bBlockOutput = false;
}

void plTestFramework::TestResultImpl(plInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  std::scoped_lock _(m_OutputMutex);

  m_Result.TestResult(m_iCurrentTestIndex, iSubTestIndex, bSuccess, fDuration);

  const plUInt32 uiMin = (plUInt32)(fDuration / 1000.0 / 60.0);
  const plUInt32 uiSec = (plUInt32)(fDuration / 1000.0 - uiMin * 60.0);
  const plUInt32 uiMS = (plUInt32)(fDuration - uiSec * 1000.0);

  plTestFramework::Output(plTestOutput::Duration, "%i:%02i:%03i", uiMin, uiSec, uiMS);

  if (iSubTestIndex == -1)
  {
    const char* szTestName = m_TestEntries[m_iCurrentTestIndex].m_szTestName;
    if (bSuccess)
    {
      m_iTestsPassed++;
      plTestFramework::Output(plTestOutput::Success, "Test '%s' succeeded (%.2f sec).", szTestName, m_fTotalTestDuration / 1000.0f);

      if (GetSettings().m_bAutoDisableSuccessfulTests)
      {
        m_TestEntries[m_iCurrentTestIndex].m_bEnableTest = false;
        plTestFramework::AutoSaveTestOrder();
      }
    }
    else
    {
      m_iTestsFailed++;
      plTestFramework::Output(plTestOutput::Error, "Test '%s' failed: %i Errors (%.2f sec).", szTestName, (plUInt32)m_Result.GetErrorMessageCount(m_iCurrentTestIndex, iSubTestIndex), m_fTotalTestDuration / 1000.0f);
    }
  }
  else
  {
    const char* szSubTestName = m_TestEntries[m_iCurrentTestIndex].m_SubTests[iSubTestIndex].m_szSubTestName;
    if (bSuccess)
    {
      plTestFramework::Output(plTestOutput::Success, "Sub-Test '%s' succeeded (%.2f sec).", szSubTestName, m_fTotalSubTestDuration / 1000.0f);

      if (GetSettings().m_bAutoDisableSuccessfulTests)
      {
        m_TestEntries[m_iCurrentTestIndex].m_SubTests[iSubTestIndex].m_bEnableTest = false;
        plTestFramework::AutoSaveTestOrder();
      }
    }
    else
    {
      plTestFramework::Output(plTestOutput::Error, "Sub-Test '%s' failed: %i Errors (%.2f sec).", szSubTestName, (plUInt32)m_Result.GetErrorMessageCount(m_iCurrentTestIndex, iSubTestIndex), m_fTotalSubTestDuration / 1000.0f);
    }
  }
}

void plTestFramework::FlushAsserts()
{
  std::scoped_lock _(m_OutputMutex);
  m_Result.AddAsserts(m_iCurrentTestIndex, m_iCurrentSubTestIndex, s_iAssertCounter);
  s_iAssertCounter = 0;
}

void plTestFramework::ScheduleImageComparison(plUInt32 uiImageNumber, plUInt32 uiMaxError)
{
  m_bImageComparisonScheduled = true;
  m_uiMaxImageComparisonError = uiMaxError;
  m_uiComparisonImageNumber = uiImageNumber;
}

void plTestFramework::ScheduleDepthImageComparison(plUInt32 uiImageNumber, plUInt32 uiMaxError)
{
  m_bDepthImageComparisonScheduled = true;
  m_uiMaxDepthImageComparisonError = uiMaxError;
  m_uiComparisonDepthImageNumber = uiImageNumber;
}

void plTestFramework::GenerateComparisonImageName(plUInt32 uiImageNumber, plStringBuilder& ref_sImgName)
{
  const char* szTestName = GetTest(GetCurrentTestIndex())->m_szTestName;
  const char* szSubTestName = GetTest(GetCurrentTestIndex())->m_SubTests[GetCurrentSubTestIndex()].m_szSubTestName;
  GetTest(GetCurrentTestIndex())->m_pTest->MapImageNumberToString(szTestName, szSubTestName, uiImageNumber, ref_sImgName);
}

void plTestFramework::GetCurrentComparisonImageName(plStringBuilder& ref_sImgName)
{
  GenerateComparisonImageName(m_uiComparisonImageNumber, ref_sImgName);
}

void plTestFramework::SetImageReferenceFolderName(const char* szFolderName)
{
  m_sImageReferenceFolderName = szFolderName;
}

void plTestFramework::SetImageReferenceOverrideFolderName(const char* szFolderName)
{
  m_sImageReferenceOverrideFolderName = szFolderName;

  if (!m_sImageReferenceOverrideFolderName.empty())
  {
    Output(plTestOutput::Message, "Using ImageReference override folder '%s'", szFolderName);
  }
}

static const plUInt8 s_Base64EncodingTable[64] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
  'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};

static const plUInt8 BASE64_CHARS_PER_LINE = 76;

static plUInt32 GetBase64EncodedLength(plUInt32 uiInputLength, bool bInsertLineBreaks)
{
  plUInt32 outputLength = (uiInputLength + 2) / 3 * 4;

  if (bInsertLineBreaks)
  {
    outputLength += outputLength / BASE64_CHARS_PER_LINE;
  }

  return outputLength;
}


static plDynamicArray<char> ArrayToBase64(plArrayPtr<const plUInt8> in, bool bInsertLineBreaks = true)
{
  plDynamicArray<char> out;
  out.SetCountUninitialized(GetBase64EncodedLength(in.GetCount(), bInsertLineBreaks));

  plUInt32 offsetIn = 0;
  plUInt32 offsetOut = 0;

  plUInt32 blocksTillNewline = BASE64_CHARS_PER_LINE / 4;
  while (offsetIn < in.GetCount())
  {
    plUInt8 ibuf[3] = {0};

    plUInt32 ibuflen = plMath::Min(in.GetCount() - offsetIn, 3u);

    for (plUInt32 i = 0; i < ibuflen; ++i)
    {
      ibuf[i] = in[offsetIn++];
    }

    char obuf[4];
    obuf[0] = s_Base64EncodingTable[(ibuf[0] >> 2)];
    obuf[1] = s_Base64EncodingTable[((ibuf[0] << 4) & 0x30) | (ibuf[1] >> 4)];
    obuf[2] = s_Base64EncodingTable[((ibuf[1] << 2) & 0x3c) | (ibuf[2] >> 6)];
    obuf[3] = s_Base64EncodingTable[(ibuf[2] & 0x3f)];

    if (ibuflen >= 3)
    {
      out[offsetOut++] = obuf[0];
      out[offsetOut++] = obuf[1];
      out[offsetOut++] = obuf[2];
      out[offsetOut++] = obuf[3];
    }
    else // need to pad up to 4
    {
      switch (ibuflen)
      {
        case 1:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = '=';
          out[offsetOut++] = '=';
          break;
        case 2:
          out[offsetOut++] = obuf[0];
          out[offsetOut++] = obuf[1];
          out[offsetOut++] = obuf[2];
          out[offsetOut++] = '=';
          break;
      }
    }

    if (--blocksTillNewline == 0)
    {
      if (bInsertLineBreaks)
      {
        out[offsetOut++] = '\n';
      }
      blocksTillNewline = 19;
    }
  }

  PLASMA_ASSERT_DEV(offsetOut == out.GetCount(), "All output data should have been written");
  return out;
}

static void AppendImageData(plStringBuilder& ref_sOutput, plImage& ref_img)
{
  plImageFileFormat* format = plImageFileFormat::GetWriterFormat("png");
  PLASMA_ASSERT_DEV(format != nullptr, "No PNG writer found");

  plDynamicArray<plUInt8> imgData;
  plMemoryStreamContainerWrapperStorage<plDynamicArray<plUInt8>> storage(&imgData);
  plMemoryStreamWriter writer(&storage);
  format->WriteImage(writer, ref_img, "png").IgnoreResult();

  plDynamicArray<char> imgDataBase64 = ArrayToBase64(imgData.GetArrayPtr());
  plStringView imgDataBase64StringView(imgDataBase64.GetArrayPtr().GetPtr(), imgDataBase64.GetArrayPtr().GetEndPtr());
  ref_sOutput.AppendFormat("data:image/png;base64,{0}", imgDataBase64StringView);
}

void plTestFramework::WriteImageDiffHtml(const char* szFileName, plImage& ref_referenceImgRgb, plImage& ref_referenceImgAlpha, plImage& ref_capturedImgRgb, plImage& ref_capturedImgAlpha, plImage& ref_diffImgRgb, plImage& ref_diffImgAlpha, plUInt32 uiError, plUInt32 uiThreshold, plUInt8 uiMinDiffRgb, plUInt8 uiMaxDiffRgb,
  plUInt8 uiMinDiffAlpha, plUInt8 uiMaxDiffAlpha)
{

  plFileWriter outputFile;
  if (outputFile.Open(szFileName).Failed())
  {
    plTestFramework::Output(plTestOutput::Warning, "Could not open HTML diff file \"%s\" for writing.", szFileName);
    return;
  }

  plStringBuilder output;
  output.Append("<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<!DOCTYPE html PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
                "<HTML> <HEAD>\n");
  const char* szTestName = GetTest(GetCurrentTestIndex())->m_szTestName;
  const char* szSubTestName = GetTest(GetCurrentTestIndex())->m_SubTests[GetCurrentSubTestIndex()].m_szSubTestName;
  output.AppendFormat("<TITLE>{} - {}</TITLE>\n", szTestName, szSubTestName);
  output.Append("<script type = \"text/javascript\">\n"
                "function showReferenceImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'none'\n"
                "    document.getElementById('image_current_a').style.display = 'none'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Reference Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Reference Image Alpha'\n"
                "}\n"
                "function showCurrentImage()\n"
                "{\n"
                "    document.getElementById('image_current_rgb').style.display = 'inline-block'\n"
                "    document.getElementById('image_current_a').style.display = 'inline-block'\n"
                "    document.getElementById('image_reference_rgb').style.display = 'none'\n"
                "    document.getElementById('image_reference_a').style.display = 'none'\n"
                "    document.getElementById('image_caption_rgb').innerHTML = 'Displaying: Current Image RGB'\n"
                "    document.getElementById('image_caption_a').innerHTML = 'Displaying: Current Image Alpha'\n"
                "}\n"
                "function imageover()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "function imageout()\n"
                "{\n"
                "    var mode = document.querySelector('input[name=\"image_interaction_mode\"]:checked').value\n"
                "    if (mode == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "}\n"
                "function handleModeClick(clickedItem)\n"
                "{\n"
                "    if (clickedItem.value == 'current_image' || clickedItem.value == 'interactive')\n"
                "    {\n"
                "        showCurrentImage()\n"
                "    }\n"
                "    else if (clickedItem.value == 'reference_image')\n"
                "    {\n"
                "        showReferenceImage()\n"
                "    }\n"
                "}\n"
                "</script>\n"
                "</HEAD>\n"
                "<BODY bgcolor=\"#ccdddd\">\n"
                "<div style=\"line-height: 1.5; margin-top: 0px; margin-left: 10px; font-family: sans-serif;\">\n");

  output.AppendFormat("<b>Test result for \"{} > {}\" from ", szTestName, szSubTestName);
  plDateTime dateTime(plTimestamp::CurrentTimestamp());
  output.AppendFormat("{}-{}-{} {}:{}:{}</b><br>\n", dateTime.GetYear(), plArgI(dateTime.GetMonth(), 2, true), plArgI(dateTime.GetDay(), 2, true), plArgI(dateTime.GetHour(), 2, true), plArgI(dateTime.GetMinute(), 2, true), plArgI(dateTime.GetSecond(), 2, true));

  output.Append("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\">\n");

  if (m_ImageDiffExtraInfoCallback)
  {
    plDynamicArray<std::pair<plString, plString>> extraInfo = m_ImageDiffExtraInfoCallback();

    for (const auto& labelValuePair : extraInfo)
    {
      output.AppendFormat("<tr>\n"
                          "<td>{}:</td>\n"
                          "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                          "</tr>\n",
        labelValuePair.first, labelValuePair.second);
    }
  }

  output.AppendFormat("<tr>\n"
                      "<td>Error metric:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiError);
  output.AppendFormat("<tr>\n"
                      "<td>Error threshold:</td>\n"
                      "<td align=\"right\" style=\"padding-left: 2em;\">{}</td>\n"
                      "</tr>\n",
    uiThreshold);
  output.Append("</table>\n"
                "<div style=\"margin-top: 0.5em; margin-bottom: -0.75em\">\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"interactive\" "
                "checked=\"checked\"> Mouse-Over Image Switching\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"current_image\"> "
                "Current Image\n"
                "    <input type=\"radio\" name=\"image_interaction_mode\" onclick=\"handleModeClick(this)\" value=\"reference_image\"> "
                "Reference Image\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", ref_capturedImgRgb.GetWidth());

  output.Append("<p id=\"image_caption_rgb\">Displaying: Current Image RGB</p>\n"

                "<div style=\"block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_rgb\" alt=\"Captured Image RGB\" src=\"");
  AppendImageData(output, ref_capturedImgRgb);
  output.Append("\" />\n"
                "<img id=\"image_reference_rgb\" style=\"display: none\" alt=\"Reference Image RGB\" src=\"");
  AppendImageData(output, ref_referenceImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"display: block;\">\n");
  output.AppendFormat("<p>RGB Difference (min: {}, max: {}):</p>\n", uiMinDiffRgb, uiMaxDiffRgb);
  output.Append("<img alt=\"Diff Image RGB\" src=\"");
  AppendImageData(output, ref_diffImgRgb);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n");

  output.AppendFormat("<div style=\"width:{}px;display: inline-block;\">\n", ref_capturedImgAlpha.GetWidth());

  output.Append("<p id=\"image_caption_a\">Displaying: Current Image Alpha</p>\n"
                "<div style=\"display: block;\" onmouseover=\"imageover()\" onmouseout=\"imageout()\">\n"
                "<img id=\"image_current_a\" alt=\"Captured Image Alpha\" src=\"");
  AppendImageData(output, ref_capturedImgAlpha);
  output.Append("\" />\n"
                "<img id=\"image_reference_a\" style=\"display: none\" alt=\"Reference Image Alpha\" src=\"");
  AppendImageData(output, ref_referenceImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "<div style=\"px;display: block;\">\n");
  output.AppendFormat("<p>Alpha Difference (min: {}, max: {}):</p>\n", uiMinDiffAlpha, uiMaxDiffAlpha);
  output.Append("<img alt=\"Diff Image Alpha\" src=\"");
  AppendImageData(output, ref_diffImgAlpha);
  output.Append("\" />\n"
                "</div>\n"
                "</div>\n"
                "</div>\n"
                "</BODY> </HTML>");

  outputFile.WriteBytes(output.GetData(), output.GetCharacterCount()).IgnoreResult();
  outputFile.Close();
}

bool plTestFramework::PerformImageComparison(plStringBuilder sImgName, const plImage& img, plUInt32 uiMaxError, bool bIsLineImage, char* szErrorMsg)
{
  plImage imgRgba;
  if (plImageConversion::Convert(img, imgRgba, plImageFormat::R8G8B8A8_UNORM).Failed())
  {
    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Captured Image '%s' could not be converted to RGBA8", sImgName.GetData());
    return false;
  }

  plStringBuilder sImgPathReference, sImgPathResult;

  if (!m_sImageReferenceOverrideFolderName.empty())
  {
    sImgPathReference.Format("{0}/{1}.png", m_sImageReferenceOverrideFolderName.c_str(), sImgName);

    if (!plFileSystem::ExistsFile(sImgPathReference))
    {
      // try the regular path
      sImgPathReference.Clear();
    }
  }

  if (sImgPathReference.IsEmpty())
  {
    sImgPathReference.Format("{0}/{1}.png", m_sImageReferenceFolderName.c_str(), sImgName);
  }

  sImgPathResult.Format(":imgout/Images_Result/{0}.png", sImgName);

  // if a previous output image exists, get rid of it
  plFileSystem::DeleteFile(sImgPathResult);

  plImage imgExp, imgExpRgba;
  if (imgExp.LoadFrom(sImgPathReference).Failed())
  {
    imgRgba.SaveTo(sImgPathResult).IgnoreResult();

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be read", sImgPathReference.GetData());
    return false;
  }

  if (plImageConversion::Convert(imgExp, imgExpRgba, plImageFormat::R8G8B8A8_UNORM).Failed())
  {
    imgRgba.SaveTo(sImgPathResult).IgnoreResult();

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' could not be converted to RGBA8", sImgPathReference.GetData());
    return false;
  }

  if (imgRgba.GetWidth() != imgExpRgba.GetWidth() || imgRgba.GetHeight() != imgExpRgba.GetHeight())
  {
    imgRgba.SaveTo(sImgPathResult).IgnoreResult();

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Comparison Image '%s' size (%ix%i) does not match captured image size (%ix%i)", sImgPathReference.GetData(), imgExpRgba.GetWidth(), imgExpRgba.GetHeight(), imgRgba.GetWidth(), imgRgba.GetHeight());
    return false;
  }

  plImage imgDiffRgba;
  plImageUtils::ComputeImageDifferenceABS(imgExpRgba, imgRgba, imgDiffRgba);

  const plUInt32 uiMeanError = plImageUtils::ComputeMeanSquareError(imgDiffRgba, 32);

  if (uiMeanError > uiMaxError)
  {
    imgRgba.SaveTo(sImgPathResult).IgnoreResult();

    plUInt8 uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha;
    plImageUtils::Normalize(imgDiffRgba, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

    plImage imgDiffRgb;
    plImageConversion::Convert(imgDiffRgba, imgDiffRgb, plImageFormat::R8G8B8_UNORM).IgnoreResult();

    plStringBuilder sImgDiffName;
    sImgDiffName.Format(":imgout/Images_Diff/{0}.png", sImgName);
    imgDiffRgb.SaveTo(sImgDiffName).IgnoreResult();

    plImage imgDiffAlpha;
    plImageUtils::ExtractAlphaChannel(imgDiffRgba, imgDiffAlpha);

    plStringBuilder sImgDiffAlphaName;
    sImgDiffAlphaName.Format(":imgout/Images_Diff/{0}_alpha.png", sImgName);
    imgDiffAlpha.SaveTo(sImgDiffAlphaName).IgnoreResult();

    plImage imgExpRgb;
    plImageConversion::Convert(imgExpRgba, imgExpRgb, plImageFormat::R8G8B8_UNORM).IgnoreResult();
    plImage imgExpAlpha;
    plImageUtils::ExtractAlphaChannel(imgExpRgba, imgExpAlpha);

    plImage imgRgb;
    plImageConversion::Convert(imgRgba, imgRgb, plImageFormat::R8G8B8_UNORM).IgnoreResult();
    plImage imgAlpha;
    plImageUtils::ExtractAlphaChannel(imgRgba, imgAlpha);

    plStringBuilder sDiffHtmlPath;
    sDiffHtmlPath.Format(":imgout/Html_Diff/{0}.html", sImgName);
    WriteImageDiffHtml(sDiffHtmlPath, imgExpRgb, imgExpAlpha, imgRgb, imgAlpha, imgDiffRgb, imgDiffAlpha, uiMeanError, uiMaxError, uiMinDiffRgb, uiMaxDiffRgb, uiMinDiffAlpha, uiMaxDiffAlpha);

    safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Error: Image Comparison Failed: MSE of %u exceeds threshold of %u for image '%s'.", uiMeanError, uiMaxError, sImgName.GetData());

    plStringBuilder sDataDirRelativePath;
    plFileSystem::ResolvePath(sDiffHtmlPath, nullptr, &sDataDirRelativePath).IgnoreResult();
    plTestFramework::Output(plTestOutput::ImageDiffFile, sDataDirRelativePath);
    return false;
  }
  return true;
}

bool plTestFramework::CompareImages(plUInt32 uiImageNumber, plUInt32 uiMaxError, char* szErrorMsg, bool bIsDepthImage, bool bIsLineImage)
{
  plStringBuilder sImgName;
  GenerateComparisonImageName(uiImageNumber, sImgName);

  plImage img;
  if (bIsDepthImage)
  {
    sImgName.Append("-depth");
    if (GetTest(GetCurrentTestIndex())->m_pTest->GetDepthImage(img).Failed())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Depth image '%s' could not be captured", sImgName.GetData());
      return false;
    }
  }
  else
  {
    if (GetTest(GetCurrentTestIndex())->m_pTest->GetImage(img).Failed())
    {
      safeprintf(szErrorMsg, s_iMaxErrorMessageLength, "Image '%s' could not be captured", sImgName.GetData());
      return false;
    }
  }

  bool bImagesMatch = true;
  if (img.GetNumArrayIndices() <= 1)
  {
    bImagesMatch = PerformImageComparison(sImgName, img, uiMaxError, bIsLineImage, szErrorMsg);
  }
  else
  {
    plStringBuilder lastError;
    for (plUInt32 i = 0; i < img.GetNumArrayIndices(); ++i)
    {
      plStringBuilder subImageName;
      subImageName.AppendFormat("{0}_{1}", sImgName, i);
      if (!PerformImageComparison(subImageName, img.GetSubImageView(0, 0, i), uiMaxError, bIsLineImage, szErrorMsg))
      {
        bImagesMatch = false;
        if (!lastError.IsEmpty())
        {
          plTestFramework::Output(plTestOutput::Error, "%s", lastError.GetData());
        }
        lastError = szErrorMsg;
      }
    }
  }

  if (m_ImageComparisonCallback)
  {
    m_ImageComparisonCallback(bImagesMatch);
  }

  return bImagesMatch;
}

void plTestFramework::SetImageComparisonCallback(const ImageComparisonCallback& callback)
{
  m_ImageComparisonCallback = callback;
}

plResult plTestFramework::CaptureRegressionStat(plStringView sTestName, plStringView sName, plStringView sUnit, float value, plInt32 iTestId)
{
  plStringBuilder strippedTestName = sTestName;
  strippedTestName.ReplaceAll(" ", "");

  plStringBuilder perTestName;
  if (iTestId < 0)
  {
    perTestName.Format("{}_{}", strippedTestName, sName);
  }
  else
  {
    perTestName.Format("{}_{}_{}", strippedTestName, sName, iTestId);
  }

  {
    plStringBuilder regression;
    // The 6 floating point digits are forced as per a requirement of the CI
    // feature that parses these values.
    regression.Format("[test][REGRESSION:{}:{}:{}]", perTestName, sUnit, plArgF(value, 6));
    plLog::Info(regression);
  }

  return PLASMA_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// plTestFramework static functions
////////////////////////////////////////////////////////////////////////

void plTestFramework::Output(plTestOutput::Enum type, const char* szMsg, ...)
{
  va_list args;
  va_start(args, szMsg);

  OutputArgs(type, szMsg, args);

  va_end(args);
}

void plTestFramework::OutputArgs(plTestOutput::Enum type, const char* szMsg, va_list szArgs)
{
  // format the output text
  char szBuffer[1024 * 10];
  plInt32 pos = 0;

  if (plTestFramework::s_LogTimestampMode != plLog::TimestampMode::None)
  {
    if (type == plTestOutput::BeginBlock || type == plTestOutput::EndBlock || type == plTestOutput::ImportantInfo || type == plTestOutput::Details || type == plTestOutput::Success || type == plTestOutput::Message || type == plTestOutput::Warning || type == plTestOutput::Error ||
        type == plTestOutput::FinalResult)
    {
      plStringBuilder timestamp;

      plLog::GenerateFormattedTimestamp(plTestFramework::s_LogTimestampMode, timestamp);
      pos = plStringUtils::snprintf(szBuffer, PLASMA_ARRAY_SIZE(szBuffer), "%s", timestamp.GetData());
    }
  }
  plStringUtils::vsnprintf(szBuffer + pos, PLASMA_ARRAY_SIZE(szBuffer) - pos, szMsg, szArgs);

  GetInstance()->OutputImpl(type, szBuffer);
}

void plTestFramework::Error(const char* szError, const char* szFile, plInt32 iLine, const char* szFunction, plStringView sMsg, ...)
{
  va_list args;
  va_start(args, sMsg);

  Error(szError, szFile, iLine, szFunction, sMsg, args);

  va_end(args);
}

void plTestFramework::Error(const char* szError, const char* szFile, plInt32 iLine, const char* szFunction, plStringView sMsg, va_list szArgs)
{
  // format the output text
  char szBuffer[1024 * 10];
  plStringUtils::vsnprintf(szBuffer, PLASMA_ARRAY_SIZE(szBuffer), plString(sMsg).GetData(), szArgs);

  GetInstance()->ErrorImpl(szError, szFile, iLine, szFunction, szBuffer);
}

void plTestFramework::TestResult(plInt32 iSubTestIndex, bool bSuccess, double fDuration)
{
  GetInstance()->TestResultImpl(iSubTestIndex, bSuccess, fDuration);
}

////////////////////////////////////////////////////////////////////////
// PLASMA_TEST_... macro functions
////////////////////////////////////////////////////////////////////////

#define OUTPUT_TEST_ERROR                                                        \
  {                                                                              \
    va_list args;                                                                \
    va_start(args, szMsg);                                                       \
    plTestFramework::Error(szErrorText, szFile, iLine, szFunction, szMsg, args); \
    PLASMA_TEST_DEBUG_BREAK                                                          \
    va_end(args);                                                                \
    return PLASMA_FAILURE;                                                           \
  }

bool plTestBool(bool bCondition, const char* szErrorText, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  if (!bCondition)
  {
    // if the test breaks here, go one up in the callstack to see where it exactly failed
    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

bool plTestResult(plResult condition, const char* szErrorText, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  if (condition.Failed())
  {
    // if the test breaks here, go one up in the callstack to see where it exactly failed
    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

bool plTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  const double fD = f1 - f2;

  if (fD < -fEps || fD > +fEps)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%.8f) does not equal '%s' (%.8f) within an epsilon of %.8f", szF1, f1, szF2, f2, fEps);

    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

bool plTestInt(plInt64 i1, plInt64 i2, const char* szI1, const char* szI2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  if (i1 != i2)
  {
    char szErrorText[256];
    safeprintf(szErrorText, 256, "Failure: '%s' (%lli) does not equal '%s' (%lli)", szI1, i1, szI2, i2);

    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

bool plTestWString(std::wstring s1, std::wstring s2, const char* szWString1, const char* szWString2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  if (s1 != s2)
  {
    char szErrorText[2048];
    safeprintf(szErrorText, 2048, "Failure: '%s' (%s) does not equal '%s' (%s)", szWString1, plStringUtf8(s1.c_str()).GetData(), szWString2, plStringUtf8(s2.c_str()).GetData());

    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

bool plTestString(plStringView s1, plStringView s2, const char* szString1, const char* szString2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  if (s1 != s2)
  {
    plStringBuilder ss1 = s1;
    plStringBuilder ss2 = s2;

    char szErrorText[2048];
    safeprintf(szErrorText, 2048, "Failure: '%s' (%s) does not equal '%s' (%s)", szString1, ss1.GetData(), szString2, ss2.GetData());

    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

bool plTestVector(plVec4d v1, plVec4d v2, double fEps, const char* szCondition, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  char szErrorText[256];

  if (!plMath::IsEqual(v1.x, v2.x, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.x (%.8f) does not equal v2.x (%.8f) within an epsilon of %.8f", szCondition, v1.x, v2.x, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!plMath::IsEqual(v1.y, v2.y, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.y (%.8f) does not equal v2.y (%.8f) within an epsilon of %.8f", szCondition, v1.y, v2.y, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!plMath::IsEqual(v1.z, v2.z, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.z (%.8f) does not equal v2.z (%.8f) within an epsilon of %.8f", szCondition, v1.z, v2.z, fEps);

    OUTPUT_TEST_ERROR
  }

  if (!plMath::IsEqual(v1.w, v2.w, fEps))
  {
    safeprintf(szErrorText, 256, "Failure: '%s' - v1.w (%.8f) does not equal v2.w (%.8f) within an epsilon of %.8f", szCondition, v1.w, v2.w, fEps);

    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

bool plTestFiles(const char* szFile1, const char* szFile2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  char szErrorText[s_iMaxErrorMessageLength];

  plFileReader ReadFile1;
  plFileReader ReadFile2;

  if (ReadFile1.Open(szFile1) == PLASMA_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile1);

    OUTPUT_TEST_ERROR
  }
  else if (ReadFile2.Open(szFile2) == PLASMA_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile2);

    OUTPUT_TEST_ERROR
  }

  else if (ReadFile1.GetFileSize() != ReadFile2.GetFileSize())
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File sizes do not match: '%s' (%llu Bytes) and '%s' (%llu Bytes)", szFile1, ReadFile1.GetFileSize(), szFile2, ReadFile2.GetFileSize());

    OUTPUT_TEST_ERROR
  }
  else
  {
    while (true)
    {
      plUInt8 uiTemp1[512];
      plUInt8 uiTemp2[512];
      const plUInt64 uiRead1 = ReadFile1.ReadBytes(uiTemp1, 512);
      const plUInt64 uiRead2 = ReadFile2.ReadBytes(uiTemp2, 512);

      if (uiRead1 != uiRead2)
      {
        safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Files could not read same amount of data: '%s' and '%s'", szFile1, szFile2);

        OUTPUT_TEST_ERROR
      }
      else
      {
        if (uiRead1 == 0)
          break;

        if (memcmp(uiTemp1, uiTemp2, (size_t)uiRead1) != 0)
        {
          safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Files contents do not match: '%s' and '%s'", szFile1, szFile2);

          OUTPUT_TEST_ERROR
        }
      }
    }
  }

  return PLASMA_SUCCESS;
}

bool plTestTextFiles(const char* szFile1, const char* szFile2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  plTestFramework::s_iAssertCounter++;

  char szErrorText[s_iMaxErrorMessageLength];

  plFileReader ReadFile1;
  plFileReader ReadFile2;

  if (ReadFile1.Open(szFile1) == PLASMA_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile1);

    OUTPUT_TEST_ERROR
  }
  else if (ReadFile2.Open(szFile2) == PLASMA_FAILURE)
  {
    safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: File '%s' could not be read.", szFile2);

    OUTPUT_TEST_ERROR
  }
  else
  {
    plStringBuilder sFile1;
    sFile1.ReadAll(ReadFile1);
    sFile1.ReplaceAll("\r\n", "\n");

    plStringBuilder sFile2;
    sFile2.ReadAll(ReadFile2);
    sFile2.ReplaceAll("\r\n", "\n");

    if (sFile1 != sFile2)
    {
      safeprintf(szErrorText, s_iMaxErrorMessageLength, "Failure: Text files contents do not match: '%s' and '%s'", szFile1, szFile2);

      OUTPUT_TEST_ERROR
    }
  }

  return PLASMA_SUCCESS;
}

bool plTestImage(plUInt32 uiImageNumber, plUInt32 uiMaxError, bool bIsDepthImage, bool bIsLineImage, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...)
{
  char szErrorText[s_iMaxErrorMessageLength] = "";

  if (!plTestFramework::GetInstance()->CompareImages(uiImageNumber, uiMaxError, szErrorText, bIsDepthImage, bIsLineImage))
  {
    OUTPUT_TEST_ERROR
  }

  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_TestFramework);
