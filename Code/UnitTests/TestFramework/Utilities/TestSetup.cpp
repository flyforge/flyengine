#include <TestFramework/TestFrameworkPCH.h>

#include <TestFramework/Utilities/TestSetup.h>

#include <TestFramework/Utilities/ConsoleOutput.h>
#include <TestFramework/Utilities/HTMLOutput.h>

#include <Foundation/System/CrashHandler.h>
#include <Foundation/System/SystemInformation.h>

#ifdef PLASMA_USE_QT
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/Framework/Qt/qtTestGUI.h>
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <TestFramework/Framework/Uwp/uwpTestFramework.h>
#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <conio.h>
#endif

int plTestSetup::s_iArgc = 0;
const char** plTestSetup::s_pArgv = nullptr;

plTestFramework* plTestSetup::InitTestFramework(const char* szTestName, const char* szNiceTestName, int iArgc, const char** pArgv)
{
  s_iArgc = iArgc;
  s_pArgv = pArgv;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  if (FAILED(RoInitialize(RO_INIT_MULTITHREADED)))
  {
    std::cout << "Failed to init WinRT." << std::endl;
  }
#endif

  // without a proper file system the current working directory is pretty much useless
  std::string sTestFolder = std::string(plOSFile::GetUserDataFolder());
  if (*sTestFolder.rbegin() != '/')
    sTestFolder.append("/");
  sTestFolder.append("plEngine Tests/");
  sTestFolder.append(szTestName);

  std::string sTestDataSubFolder = "Data/UnitTests/";
  sTestDataSubFolder.append(szTestName);

#ifdef PLASMA_USE_QT
  plTestFramework* pTestFramework = new plQtTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  // Command line args in UWP are handled differently and can't be retrieved from the main function.
  plTestFramework* pTestFramework = new plUwpTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), 0, nullptr);
#else
  plTestFramework* pTestFramework = new plTestFramework(szNiceTestName, sTestFolder.c_str(), sTestDataSubFolder.c_str(), iArgc, pArgv);
#endif

  // Register some output handlers to forward all the messages to the console and to an HTML file
  pTestFramework->RegisterOutputHandler(OutputToConsole);
  pTestFramework->RegisterOutputHandler(plOutputToHTML::OutputToHTML);

  plCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(pTestFramework->GetAbsOutputPath(), szTestName);
  plCrashHandler::SetCrashHandler(&plCrashHandler_WriteMiniDump::g_Instance);

  return pTestFramework;
}

plTestAppRun plTestSetup::RunTests()
{
  plTestFramework* pTestFramework = plTestFramework::GetInstance();

  // Todo: Incorporate all the below in a virtual call of testFramework?
#ifdef PLASMA_USE_QT
  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bNoGUI)
  {
    return pTestFramework->RunTestExecutionLoop();
  }

  // Setup Qt Application

  int argc = s_iArgc;
  char** argv = const_cast<char**>(s_pArgv);

  if (qApp != nullptr)
  {
    bool ok = false;
    int iCount = qApp->property("Shared").toInt(&ok);
    PLASMA_ASSERT_DEV(ok, "Existing QApplication was not constructed by PLASMA!");
    qApp->setProperty("Shared", QVariant::fromValue(iCount + 1));
  }
  else
  {
    new QApplication(argc, argv);
    qApp->setProperty("Shared", QVariant::fromValue((int)1));
    qApp->setApplicationName(pTestFramework->GetTestName());
    plQtTestGUI::SetDarkTheme();
  }

  // Create main window
  {
    plQtTestGUI mainWindow(*static_cast<plQtTestFramework*>(pTestFramework));
    mainWindow.show();

    qApp->exec();
  }
  {
    const int iCount = qApp->property("Shared").toInt();
    if (iCount == 1)
    {
      delete qApp;
    }
    else
    {
      qApp->setProperty("Shared", QVariant::fromValue(iCount - 1));
    }
  }

  return plTestAppRun::Quit;
#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  static_cast<plUwpTestFramework*>(pTestFramework)->Run();
  return plTestAppRun::Quit;
#else
  // Run all the tests with the given order
  return pTestFramework->RunTestExecutionLoop();
#endif
}

void plTestSetup::DeInitTestFramework(bool bSilent /*= false*/)
{
  plTestFramework* pTestFramework = plTestFramework::GetInstance();

  plStartup::ShutdownCoreSystems();

  // In the UWP case we never initialized this thread for pl, so we can't do log output now.
#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_UWP)
  if (!bSilent)
  {
    plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
    plStringUtils::PrintStringLengthStatistics();
  }
#endif

  TestSettings settings = pTestFramework->GetSettings();
  if (settings.m_bKeepConsoleOpen && !bSilent)
  {
    if (plSystemInformation::IsDebuggerAttached())
    {
      std::cout << "Press the any key to continue...\n";
      fflush(stdin);
      [[maybe_unused]] int c = getchar();
    }
  }

  // This is needed as at least windows can't be bothered to write anything
  // to the output streams at all if it's not enough or the app is too fast.
  fflush(stdout);
  fflush(stderr);
  delete pTestFramework;
}

plInt32 plTestSetup::GetFailedTestCount()
{
  return plTestFramework::GetInstance()->GetTestsFailedCount();
}


PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Utilities_TestSetup);
