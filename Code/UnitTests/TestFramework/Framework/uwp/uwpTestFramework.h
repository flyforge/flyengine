#pragma once

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

#  include <TestFramework/Framework/TestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>

/// \brief Derived plTestFramework which signals the GUI to update whenever a new tests result comes in.
class PLASMA_TEST_DLL plUwpTestFramework : public plTestFramework
{
public:
  plUwpTestFramework(const char* szTestName, const char* szAbsTestDir, const char* szRelTestDataDir, int argc, const char** argv);
  virtual ~plUwpTestFramework();

  plUwpTestFramework(plUwpTestFramework&) = delete;
  void operator=(plUwpTestFramework&) = delete;

  void Run();
};

#endif
