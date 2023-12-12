#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>

plApplication::plApplication(plStringView sAppName)
  : m_sAppName(sAppName)
{
}

plApplication::~plApplication() = default;

void plApplication::SetApplicationName(plStringView sAppName)
{
  m_sAppName = sAppName;
}

plCommandLineOptionBool opt_WaitForDebugger("app", "-WaitForDebugger", "If specified, the application will wait at startup until a debugger is attached.", false);

plResult plApplication::BeforeCoreSystemsStartup()
{
  if (plFileSystem::DetectSdkRootDirectory().Failed())
  {
    plLog::Error("Unable to find the SDK root directory. Mounting data directories may fail.");
  }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  plRTTI::VerifyCorrectnessForAllTypes();
#endif

  if (opt_WaitForDebugger.GetOptionValue(plCommandLineOption::LogMode::AlwaysIfSpecified))
  {
    while (!plSystemInformation::IsDebuggerAttached())
    {
      plThreadUtils::Sleep(plTime::Milliseconds(1));
    }

    PLASMA_DEBUG_BREAK;
  }

  return PLASMA_SUCCESS;
}


void plApplication::SetCommandLineArguments(plUInt32 uiArgumentCount, const char** pArguments)
{
  m_uiArgumentCount = uiArgumentCount;
  m_pArguments = pArguments;

  plCommandLineUtils::GetGlobalInstance()->SetCommandLine(uiArgumentCount, pArguments, plCommandLineUtils::PreferOsArgs);
}


const char* plApplication::GetArgument(plUInt32 uiArgument) const
{
  PLASMA_ASSERT_DEV(uiArgument < m_uiArgumentCount, "There are only {0} arguments, cannot access argument {1}.", m_uiArgumentCount, uiArgument);

  return m_pArguments[uiArgument];
}


void plApplication::RequestQuit()
{
  m_bWasQuitRequested = true;
}


plApplication* plApplication::s_pApplicationInstance = nullptr;



PLASMA_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_Application);
