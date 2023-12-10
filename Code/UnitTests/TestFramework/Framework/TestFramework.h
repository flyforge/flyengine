#pragma once

#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/SimpleTest.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <TestFramework/Framework/TestResults.h>
#include <TestFramework/TestFrameworkDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <utility>

class plCommandLineUtils;

// Disable C++/CX adds.
#pragma warning(disable : 4447)

class PLASMA_TEST_DLL plTestFramework
{
public:
  plTestFramework(const char* szTestName, const char* szAbsTestOutputDir, const char* szRelTestDataDir, int iArgc, const char** pArgv);
  virtual ~plTestFramework();

  using OutputHandler = void (*)(plTestOutput::Enum, const char*);

  // Test management
  void CreateOutputFolder();
  void UpdateReferenceImages();
  const char* GetTestName() const;
  const char* GetAbsOutputPath() const;
  const char* GetRelTestDataPath() const;
  const char* GetAbsTestOrderFilePath() const;
  const char* GetAbsTestSettingsFilePath() const;
  void RegisterOutputHandler(OutputHandler handler);
  void GatherAllTests();
  void LoadTestOrder();
  void ApplyTestOrderFromCommandLine(const plCommandLineUtils& cmd);
  void LoadTestSettings();
  void AutoSaveTestOrder();
  void SaveTestOrder(const char* const szFilePath);
  void SaveTestSettings(const char* const szFilePath);
  void SetAllTestsEnabledStatus(bool bEnable);
  void SetAllFailedTestsEnabledStatus();
  // Each function on a test must not take longer than the given time or the test process will be terminated.
  void SetTestTimeout(plUInt32 uiTestTimeoutMS);
  plUInt32 GetTestTimeout() const;
  void GetTestSettingsFromCommandLine(const plCommandLineUtils& cmd);

  // Test execution
  void ResetTests();
  plTestAppRun RunTestExecutionLoop();

  void StartTests();
  void ExecuteNextTest();
  void EndTests();
  void AbortTests();

  // Test queries
  plUInt32 GetTestCount() const;
  plUInt32 GetTestEnabledCount() const;
  plUInt32 GetSubTestEnabledCount(plUInt32 uiTestIndex) const;
  const std::string& IsTestAvailable(plUInt32 uiTestIndex) const;
  bool IsTestEnabled(plUInt32 uiTestIndex) const;
  bool IsSubTestEnabled(plUInt32 uiTestIndex, plUInt32 uiSubTestIndex) const;
  void SetTestEnabled(plUInt32 uiTestIndex, bool bEnabled);
  void SetSubTestEnabled(plUInt32 uiTestIndex, plUInt32 uiSubTestIndex, bool bEnabled);

  plInt32 GetCurrentTestIndex() const { return m_iCurrentTestIndex; }
  plInt32 GetCurrentSubTestIndex() const { return m_iCurrentSubTestIndex; }
  plTestEntry* GetTest(plUInt32 uiTestIndex);
  const plTestEntry* GetTest(plUInt32 uiTestIndex) const;
  bool GetTestsRunning() const { return m_bTestsRunning; }

  const plTestEntry* GetCurrentTest() const;
  const plSubTestEntry* GetCurrentSubTest() const;

  // Global settings
  TestSettings GetSettings() const;
  void SetSettings(const TestSettings& settings);

  // Test results
  plTestFrameworkResult& GetTestResult();
  plInt32 GetTotalErrorCount() const;
  plInt32 GetTestsPassedCount() const;
  plInt32 GetTestsFailedCount() const;
  double GetTotalTestDuration() const;

  // Image comparison
  void ScheduleImageComparison(plUInt32 uiImageNumber, plUInt32 uiMaxError);
  void ScheduleDepthImageComparison(plUInt32 uiImageNumber, plUInt32 uiMaxError);
  bool IsImageComparisonScheduled() const { return m_bImageComparisonScheduled; }
  bool IsDepthImageComparisonScheduled() const { return m_bDepthImageComparisonScheduled; }
  void GenerateComparisonImageName(plUInt32 uiImageNumber, plStringBuilder& ref_sImgName);
  void GetCurrentComparisonImageName(plStringBuilder& ref_sImgName);
  void SetImageReferenceFolderName(const char* szFolderName);
  void SetImageReferenceOverrideFolderName(const char* szFolderName);

  /// \brief Writes an Html file that contains test information and an image diff view for failed image comparisons.
  void WriteImageDiffHtml(const char* szFileName, plImage& ref_referenceImgRgb, plImage& ref_referenceImgAlpha, plImage& ref_capturedImgRgb,
    plImage& ref_capturedImgAlpha, plImage& ref_diffImgRgb, plImage& ref_diffImgAlpha, plUInt32 uiError, plUInt32 uiThreshold, plUInt8 uiMinDiffRgb,
    plUInt8 uiMaxDiffRgb, plUInt8 uiMinDiffAlpha, plUInt8 uiMaxDiffAlpha);

  bool PerformImageComparison(plStringBuilder sImgName, const plImage& img, plUInt32 uiMaxError, bool bIsLineImage, char* szErrorMsg);
  bool CompareImages(plUInt32 uiImageNumber, plUInt32 uiMaxError, char* szErrorMsg, bool bIsDepthImage = false, bool bIsLineImage = false);

  /// \brief A function to be called to add extra info to image diff output, that is not available from here.
  /// E.g. device specific info like driver version.
  using ImageDiffExtraInfoCallback = std::function<plDynamicArray<std::pair<plString, plString>>()>;
  void SetImageDiffExtraInfoCallback(ImageDiffExtraInfoCallback provider);

  using ImageComparisonCallback = std::function<void(bool)>; /// \brief A function to be called after every image comparison with a bool
                                                             /// indicating if the images matched or not.
  void SetImageComparisonCallback(const ImageComparisonCallback& callback);

  static plResult CaptureRegressionStat(plStringView sTestName, plStringView sName, plStringView sUnit, float value, plInt32 iTestId = -1);

protected:
  void Initialize();
  void DeInitialize();

  /// \brief Will be called for test failures to record the location of the failure and forward the error to OutputImpl.
  virtual void ErrorImpl(const char* szError, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg);
  /// \brief Receives plLog messages (via LogWriter) as well as test-framework internal logging. Any plTestOutput::Error will
  /// cause the test to fail.
  virtual void OutputImpl(plTestOutput::Enum Type, const char* szMsg);
  virtual void TestResultImpl(plInt32 iSubTestIndex, bool bSuccess, double fDuration);
  void FlushAsserts();
  void TimeoutThread();
  void UpdateTestTimeout();

  // ignore this for now
public:
  static const char* s_szTestBlockName;
  static int s_iAssertCounter;
  static bool s_bCallstackOnAssert;
  static plLog::TimestampMode s_LogTimestampMode;

  // static functions
public:
  static PLASMA_ALWAYS_INLINE plTestFramework* GetInstance() { return s_pInstance; }

  /// \brief Returns whether to asset on test failure.
  static bool GetAssertOnTestFail();

  static void Output(plTestOutput::Enum type, const char* szMsg, ...);
  static void OutputArgs(plTestOutput::Enum type, const char* szMsg, va_list szArgs);
  static void Error(const char* szError, const char* szFile, plInt32 iLine, const char* szFunction, plStringView sMsg, ...);
  static void Error(const char* szError, const char* szFile, plInt32 iLine, const char* szFunction, plStringView sMsg, va_list szArgs);
  static void TestResult(plInt32 iSubTestIndex, bool bSuccess, double fDuration);

  // static members
private:
  static plTestFramework* s_pInstance;

private:
  std::string m_sTestName;                ///< The name of the tests being done
  std::string m_sAbsTestOutputDir;        ///< Absolute path to the output folder where results and temp data is stored
  std::string m_sRelTestDataDir;          ///< Relative path from the SDK to where the unit test data is located
  std::string m_sAbsTestOrderFilePath;    ///< Absolute path to the test order file
  std::string m_sAbsTestSettingsFilePath; ///< Absolute path to the test settings file
  plInt32 m_iErrorCount = 0;
  plInt32 m_iTestsFailed = 0;
  plInt32 m_iTestsPassed = 0;
  TestSettings m_Settings;
  std::recursive_mutex m_OutputMutex;
  std::deque<OutputHandler> m_OutputHandlers;
  std::deque<plTestEntry> m_TestEntries;
  plTestFrameworkResult m_Result;
  plAssertHandler m_PreviousAssertHandler = nullptr;
  ImageDiffExtraInfoCallback m_ImageDiffExtraInfoCallback;
  ImageComparisonCallback m_ImageComparisonCallback;

  std::mutex m_TimeoutLock;
  plUInt32 m_uiTimeoutMS = 5 * 60 * 1000; // 5 min default timeout
  bool m_bUseTimeout = false;
  bool m_bArm = false;
  std::condition_variable m_TimeoutCV;
  std::thread m_TimeoutThread;

  plInt32 m_iExecutingTest = 0;
  plInt32 m_iExecutingSubTest = 0;
  bool m_bSubTestInitialized = false;
  bool m_bAbortTests = false;
  plUInt8 m_uiPassesLeft = 0;
  double m_fTotalTestDuration = 0.0;
  double m_fTotalSubTestDuration = 0.0;
  plInt32 m_iErrorCountBeforeTest = 0;
  plUInt32 m_uiSubTestInvocationCount = 0;

  bool m_bIsInitialized = false;

  // image comparisons
  bool m_bImageComparisonScheduled = false;
  plUInt32 m_uiMaxImageComparisonError = 0;
  plUInt32 m_uiComparisonImageNumber = 0;

  bool m_bDepthImageComparisonScheduled = false;
  plUInt32 m_uiMaxDepthImageComparisonError = 0;
  plUInt32 m_uiComparisonDepthImageNumber = 0;

  std::string m_sImageReferenceFolderName = "Images_Reference";
  std::string m_sImageReferenceOverrideFolderName;

protected:
  plInt32 m_iCurrentTestIndex = -1;
  plInt32 m_iCurrentSubTestIndex = -1;
  bool m_bTestsRunning = false;
};

#ifdef PLASMA_NV_OPTIMUS
#  undef PLASMA_NV_OPTIMUS
#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/MinWindows.h>
#  define PLASMA_NV_OPTIMUS                                                          \
    extern "C"                                                                   \
    {                                                                            \
      _declspec(dllexport) plMinWindows::DWORD NvOptimusEnablement = 0x00000001; \
      _declspec(dllexport) plMinWindows::DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; \
    }
#else
#  define PLASMA_NV_OPTIMUS
#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  include <Foundation/Basics/Platform/Android/AndroidUtils.h>
#  include <android/log.h>
#  include <android/native_activity.h>
#  include <android_native_app_glue.h>


#  define PLASMA_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                                                   \
    int plAndroidMain(int argc, char** argv);                                                                              \
    extern "C" void android_main(struct android_app* app)                                                                  \
    {                                                                                                                      \
      plAndroidUtils::SetAndroidApp(app);                                                                                  \
      /* TODO: do something with the return value of plAndroidMain?  */                                                    \
      /* TODO: can we get somehow get the command line arguments to the android app? Is there even something like that? */ \
      int iReturnCode = plAndroidMain(0, nullptr);                                                                         \
      __android_log_print(ANDROID_LOG_ERROR, "plEngine", "Test framework exited with return code: '%d'", iReturnCode);     \
    }                                                                                                                      \
                                                                                                                           \
    int plAndroidMain(int argc, char** argv)                                                                               \
    {                                                                                                                      \
      plTestSetup::InitTestFramework(szTestName, szNiceTestName, 0, nullptr);                                              \
      /* Execute custom init code here by using the BEGIN/END macros directly */

#else
/// \brief Macro to define the application entry point for all test applications
#  define PLASMA_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)                    \
    /* Enables that on machines with multiple GPUs the NVIDIA GPU is preferred */           \
    PLASMA_NV_OPTIMUS                                                                           \
    PLASMA_APPLICATION_ENTRY_POINT_CODE_INJECTION                                               \
    int main(int argc, char** argv)                                                         \
    {                                                                                       \
      plTestSetup::InitTestFramework(szTestName, szNiceTestName, argc, (const char**)argv); \
      /* Execute custom init code here by using the BEGIN/END macros directly */

#endif

#if PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
#  define PLASMA_TESTFRAMEWORK_ENTRY_POINT_END()                                       \
    /* TODO: This is too big for a macro now */                                    \
    auto app = plAndroidUtils::GetAndroidApp();                                    \
    bool bRun = true;                                                              \
    while (true)                                                                   \
    {                                                                              \
      struct android_poll_source* source = nullptr;                                \
      int ident = 0;                                                               \
      int events = 0;                                                              \
      while ((ident = ALooper_pollAll(0, nullptr, &events, (void**)&source)) >= 0) \
      {                                                                            \
        if (source != nullptr)                                                     \
          source->process(app, source);                                            \
      }                                                                            \
      if (bRun && plTestSetup::RunTests() != plTestAppRun::Continue)               \
      {                                                                            \
        bRun = false;                                                              \
        ANativeActivity_finish(app->activity);                                     \
      }                                                                            \
      if (app->destroyRequested)                                                   \
      {                                                                            \
        const plInt32 iFailedTests = plTestSetup::GetFailedTestCount();            \
        plTestSetup::DeInitTestFramework();                                        \
        return iFailedTests;                                                       \
      }                                                                            \
    }                                                                              \
    }

#else
#  define PLASMA_TESTFRAMEWORK_ENTRY_POINT_END()                        \
    while (plTestSetup::RunTests() == plTestAppRun::Continue)       \
    {                                                               \
    }                                                               \
    const plInt32 iFailedTests = plTestSetup::GetFailedTestCount(); \
    plTestSetup::DeInitTestFramework();                             \
    return iFailedTests;                                            \
    }

#endif


#define PLASMA_TESTFRAMEWORK_ENTRY_POINT(szTestName, szNiceTestName)             \
  PLASMA_TESTFRAMEWORK_ENTRY_POINT_BEGIN(szTestName, szNiceTestName)             \
  /* Execute custom init code here by using the BEGIN/END macros directly */ \
  PLASMA_TESTFRAMEWORK_ENTRY_POINT_END()

/// \brief Enum for usage in PLASMA_TEST_BLOCK to enable or disable the block.
struct plTestBlock
{
  /// \brief Enum for usage in PLASMA_TEST_BLOCK to enable or disable the block.
  enum Enum
  {
    Enabled,           ///< The test block is enabled.
    Disabled,          ///< The test block will be skipped. The test framework will print a warning message, that some block is deactivated.
    DisabledNoWarning, ///< The test block will be skipped, but no warning printed. Used to deactivate 'on demand/optional' tests.
  };
};

#define safeprintf plStringUtils::snprintf

/// \brief Starts a small test block inside a larger test.
///
/// First parameter allows to quickly disable a block depending on a condition (e.g. platform).
/// Second parameter just gives it a name for better error reporting.
/// Also skipped tests are highlighted in the output, such that people can quickly see when a test is currently deactivated.
#define PLASMA_TEST_BLOCK(enable, name)                                                  \
  plTestFramework::s_szTestBlockName = name;                                         \
  if (enable == plTestBlock::Disabled)                                               \
  {                                                                                  \
    plTestFramework::s_szTestBlockName = "";                                         \
    plTestFramework::Output(plTestOutput::Warning, "Skipped Test Block '%s'", name); \
  }                                                                                  \
  else if (enable == plTestBlock::DisabledNoWarning)                                 \
  {                                                                                  \
    plTestFramework::s_szTestBlockName = "";                                         \
  }                                                                                  \
  else


/// \brief Will trigger a debug break, if the test framework is configured to do so on test failure
#define PLASMA_TEST_DEBUG_BREAK                   \
  if (plTestFramework::GetAssertOnTestFail()) \
  PLASMA_DEBUG_BREAK

#define PLASMA_TEST_FAILURE(erroroutput, msg, ...)                                                                   \
  {                                                                                                              \
    plTestFramework::Error(erroroutput, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__); \
    PLASMA_TEST_DEBUG_BREAK                                                                                          \
  }

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestBool(
  bool bCondition, const char* szErrorText, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests for a boolean condition, does not output an extra message.
#define PLASMA_TEST_BOOL(condition) PLASMA_TEST_BOOL_MSG(condition, "")

/// \brief Tests for a boolean condition, outputs a custom message on failure.
#define PLASMA_TEST_BOOL_MSG(condition, msg, ...) \
  plTestBool(condition, "Test failed: " PLASMA_STRINGIZE(condition), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestResult(
  plResult condition, const char* szErrorText, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests for a boolean condition, does not output an extra message.
#define PLASMA_TEST_RESULT(condition) PLASMA_TEST_RESULT_MSG(condition, "")

/// \brief Tests for a boolean condition, outputs a custom message on failure.
#define PLASMA_TEST_RESULT_MSG(condition, msg, ...) \
  plTestResult(condition, "Test failed: " PLASMA_STRINGIZE(condition), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestResult(
  plResult condition, const char* szErrorText, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests for a boolean condition, does not output an extra message.
#define PLASMA_TEST_RESULT(condition) PLASMA_TEST_RESULT_MSG(condition, "")

/// \brief Tests for a boolean condition, outputs a custom message on failure.
#define PLASMA_TEST_RESULT_MSG(condition, msg, ...) \
  plTestResult(condition, "Test failed: " PLASMA_STRINGIZE(condition), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests for a plStatus condition, outputs plStatus message on failure
#define PLASMA_TEST_STATUS(condition)                 \
  auto PLASMA_CONCAT(l_, PLASMA_SOURCE_LINE) = condition; \
  plTestResult(PLASMA_CONCAT(l_, PLASMA_SOURCE_LINE).m_Result, "Test failed: " PLASMA_STRINGIZE(condition), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, PLASMA_CONCAT(l_, PLASMA_SOURCE_LINE).m_sMessage)

inline double ToFloat(int f)
{
  return static_cast<double>(f);
}

inline double ToFloat(float f)
{
  return static_cast<double>(f);
}

inline double ToFloat(double f)
{
  return static_cast<double>(f);
}

PLASMA_TEST_DLL bool plTestDouble(double f1, double f2, double fEps, const char* szF1, const char* szF2, const char* szFile, plInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// \brief Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output.
#define PLASMA_TEST_FLOAT(f1, f2, epsilon) PLASMA_TEST_FLOAT_MSG(f1, f2, epsilon, "")

/// \brief Tests two floats for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define PLASMA_TEST_FLOAT_MSG(f1, f2, epsilon, msg, ...)                                                                                               \
  plTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), PLASMA_STRINGIZE(f1), PLASMA_STRINGIZE(f2), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, \
    msg, ##__VA_ARGS__)


//////////////////////////////////////////////////////////////////////////

/// \brief Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output.
#define PLASMA_TEST_DOUBLE(f1, f2, epsilon) PLASMA_TEST_DOUBLE_MSG(f1, f2, epsilon, "")

/// \brief Tests two doubles for equality, within a given epsilon. On failure both actual and expected values are output, also a custom
/// message is printed.
#define PLASMA_TEST_DOUBLE_MSG(f1, f2, epsilon, msg, ...)                                                                                              \
  plTestDouble(ToFloat(f1), ToFloat(f2), ToFloat(epsilon), PLASMA_STRINGIZE(f1), PLASMA_STRINGIZE(f2), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, \
    msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestInt(
  plInt64 i1, plInt64 i2, const char* szI1, const char* szI2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests two ints for equality. On failure both actual and expected values are output.
#define PLASMA_TEST_INT(i1, i2) PLASMA_TEST_INT_MSG(i1, i2, "")

/// \brief Tests two ints for equality. On failure both actual and expected values are output, also a custom message is printed.
#define PLASMA_TEST_INT_MSG(i1, i2, msg, ...) \
  plTestInt(i1, i2, PLASMA_STRINGIZE(i1), PLASMA_STRINGIZE(i2), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestString(plStringView s1, plStringView s2, const char* szString1, const char* szString2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests two strings for equality. On failure both actual and expected values are output.
#define PLASMA_TEST_STRING(i1, i2) PLASMA_TEST_STRING_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define PLASMA_TEST_STRING_MSG(s1, s2, msg, ...)                                                                                                     \
  plTestString(static_cast<plStringView>(s1), static_cast<plStringView>(s2), PLASMA_STRINGIZE(s1), PLASMA_STRINGIZE(s2), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, \
    PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestWString(std::wstring s1, std::wstring s2, const char* szString1, const char* szString2, const char* szFile, plInt32 iLine,
  const char* szFunction, const char* szMsg, ...);

/// \brief Tests two strings for equality. On failure both actual and expected values are output.
#define PLASMA_TEST_WSTRING(i1, i2) PLASMA_TEST_WSTRING_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed.
#define PLASMA_TEST_WSTRING_MSG(s1, s2, msg, ...)                                                                                         \
  plTestWString(static_cast<const wchar_t*>(s1), static_cast<const wchar_t*>(s2), PLASMA_STRINGIZE(s1), PLASMA_STRINGIZE(s2), PLASMA_SOURCE_FILE, \
    PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two strings for equality. On failure both actual and expected values are output. Does not embed the original expression to
/// work around issues with the current code page and unicode literals.
#define PLASMA_TEST_STRING_UNICODE(i1, i2) PLASMA_TEST_STRING_UNICODE_MSG(i1, i2, "")

/// \brief Tests two strings for equality. On failure both actual and expected values are output, also a custom message is printed. Does not
/// embed the original expression to work around issues with the current code page and unicode literals.
#define PLASMA_TEST_STRING_UNICODE_MSG(s1, s2, msg, ...) \
  plTestString(                                      \
    static_cast<const char*>(s1), static_cast<const char*>(s2), "", "", PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestVector(
  plVec4d v1, plVec4d v2, double fEps, const char* szCondition, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Tests two plVec2's for equality, using some epsilon. On failure both actual and expected values are output.
#define PLASMA_TEST_VEC2(i1, i2, epsilon) PLASMA_TEST_VEC2_MSG(i1, i2, epsilon, "")

/// \brief Tests two plVec2's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define PLASMA_TEST_VEC2_MSG(r1, r2, epsilon, msg, ...)                                                                                \
  plTestVector(plVec4d(ToFloat((r1).x), ToFloat((r1).y), 0, 0), plVec4d(ToFloat((r2).x), ToFloat((r2).y), 0, 0), ToFloat(epsilon), \
    PLASMA_STRINGIZE(r1) " == " PLASMA_STRINGIZE(r2), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two plVec3's for equality, using some epsilon. On failure both actual and expected values are output.
#define PLASMA_TEST_VEC3(i1, i2, epsilon) PLASMA_TEST_VEC3_MSG(i1, i2, epsilon, "")

/// \brief Tests two plVec3's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define PLASMA_TEST_VEC3_MSG(r1, r2, epsilon, msg, ...)                                                                                          \
  plTestVector(plVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), 0), plVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), 0), \
    ToFloat(epsilon), PLASMA_STRINGIZE(r1) " == " PLASMA_STRINGIZE(r2), PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

/// \brief Tests two plVec4's for equality, using some epsilon. On failure both actual and expected values are output.
#define PLASMA_TEST_VEC4(i1, i2, epsilon) PLASMA_TEST_VEC4_MSG(i1, i2, epsilon, "")

/// \brief Tests two plVec4's for equality. On failure both actual and expected values are output, also a custom message is printed.
#define PLASMA_TEST_VEC4_MSG(r1, r2, epsilon, msg, ...)                                                                                          \
  plTestVector(plVec4d(ToFloat((r1).x), ToFloat((r1).y), ToFloat((r1).z), ToFloat((r1).w)),                                                  \
    plVec4d(ToFloat((r2).x), ToFloat((r2).y), ToFloat((r2).z), ToFloat((r2).w)), ToFloat(epsilon), PLASMA_STRINGIZE(r1) " == " PLASMA_STRINGIZE(r2), \
    PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestFiles(
  const char* szFile1, const char* szFile2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define PLASMA_TEST_FILES(szFile1, szFile2, msg, ...) \
  plTestFiles(szFile1, szFile2, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

PLASMA_TEST_DLL bool plTestTextFiles(
  const char* szFile1, const char* szFile2, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

#define PLASMA_TEST_TEXT_FILES(szFile1, szFile2, msg, ...) \
  plTestTextFiles(szFile1, szFile2, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

//////////////////////////////////////////////////////////////////////////

PLASMA_TEST_DLL bool plTestImage(
  plUInt32 uiImageNumber, plUInt32 uiMaxError, bool bIsDepthImage, bool bIsLineImage, const char* szFile, plInt32 iLine, const char* szFunction, const char* szMsg, ...);

/// \brief Same as PLASMA_TEST_IMAGE_MSG but uses an empty error message.
#define PLASMA_TEST_IMAGE(ImageNumber, MaxError) PLASMA_TEST_IMAGE_MSG(ImageNumber, MaxError, "")

/// \brief Same as PLASMA_TEST_DEPTH_IMAGE_MSG but uses an empty error message.
#define PLASMA_TEST_DEPTH_IMAGE(ImageNumber, MaxError) PLASMA_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, "")

/// \brief Same as PLASMA_TEST_LINE_IMAGE_MSG but uses an empty error message.
#define PLASMA_TEST_LINE_IMAGE(ImageNumber, MaxError) PLASMA_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, "")

/// \brief Executes an image comparison right now.
///
/// The reference image is read from disk.
/// The path to the reference image is constructed from the test and sub-test name and the 'ImageNumber'.
/// One can, for instance, use the 'invocation count' that is passed to plTestBaseClass::RunSubTest() as the ImageNumber,
/// but any other integer is fine as well.
///
/// The current image to compare is taken from plTestBaseClass::GetImage().
/// Rendering tests typically override this function to return the result of the currently rendered frame.
///
/// 'MaxError' specifies the maximum mean-square error that is still considered acceptable
/// between the reference image and the current image.
///
/// Use the * DEPTH * variant if a depth buffer comparison should be requested.
///
/// \note Some tests need to know at the start, whether an image comparison will be done at the end, so they
/// can capture the image first. For such use cases, use PLASMA_SCHEDULE_IMAGE_TEST at the start of a sub-test instead.
#define PLASMA_TEST_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  plTestImage(ImageNumber, MaxError, false, false, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

#define PLASMA_TEST_DEPTH_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  plTestImage(ImageNumber, MaxError, true, false, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// \brief Same as PLASMA_TEST_IMAGE_MSG, but allows for pixels to shift in a 1-pixel radius to account for different line rasterization of GPU vendors.
#define PLASMA_TEST_LINE_IMAGE_MSG(ImageNumber, MaxError, msg, ...) \
  plTestImage(ImageNumber, MaxError, false, true, PLASMA_SOURCE_FILE, PLASMA_SOURCE_LINE, PLASMA_SOURCE_FUNCTION, msg, ##__VA_ARGS__)

/// \brief Schedules an PLASMA_TEST_IMAGE to be executed after the current sub-test execution finishes.
///
/// Call this at the beginning of a sub-test, to automatically execute an image comparison when it is finished.
/// Calling plTestFramework::IsImageComparisonScheduled() will now return true.
///
/// To support image comparisons, tests derived from plTestBaseClass need to provide the current image through plTestBaseClass::GetImage().
/// To support 'scheduled' image comparisons, the class should poll plTestFramework::IsImageComparisonScheduled() every step and capture the
/// image when needed.
///
/// Use the * DEPTH * variant if a depth buffer comparison is intended.
///
/// \note Scheduling image comparisons is an optimization to only capture data when necessary, instead of capturing it every single frame.
#define PLASMA_SCHEDULE_IMAGE_TEST(ImageNumber, MaxError) plTestFramework::GetInstance()->ScheduleImageComparison(ImageNumber, MaxError);

#define PLASMA_SCHEDULE_DEPTH_IMAGE_TEST(ImageNumber, MaxError) plTestFramework::GetInstance()->ScheduleDepthImageComparison(ImageNumber, MaxError);
