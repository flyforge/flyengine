#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Application/Application.h>
#  include <Foundation/Application/Implementation/uwp/Application_uwp.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Strings/StringConversion.h>

// Disable warning produced by CppWinRT
#  pragma warning(disable : 5205)
#  include <winrt/Windows.ApplicationModel.Activation.h>
#  include <winrt/Windows.ApplicationModel.Core.h>
#  include <winrt/Windows.Foundation.Collections.h>
#  include <winrt/Windows.Foundation.h>
#  include <winrt/Windows.UI.Core.h>

using namespace winrt::Windows::ApplicationModel::Core;

plUwpApplication::plUwpApplication(plApplication* application)
  : m_application(application)
{
}

plUwpApplication::~plUwpApplication() {}

winrt::Windows::ApplicationModel::Core::IFrameworkView plUwpApplication::CreateView()
{
  return this->get_strong().try_as<winrt::Windows::ApplicationModel::Core::IFrameworkView>();
}

void plUwpApplication::Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& applicationView)
{
  applicationView.Activated({this, &plUwpApplication::OnViewActivated});
}

void plUwpApplication::SetWindow(winrt::Windows::UI::Core::CoreWindow const& window)
{
}

void plUwpApplication::Load(winrt::hstring const& entryPoint)
{
}

void plUwpApplication::Run()
{
  if (plRun_Startup(m_application).Succeeded())
  {
    auto window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread();
    window.Activate();

    plRun_MainLoop(m_application);
  }
  plRun_Shutdown(m_application);
}

void plUwpApplication::Uninitialize()
{
}

void plUwpApplication::OnViewActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& sender, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs const& args)
{
  sender.Activated(m_activateRegistrationToken);

  if (args.Kind() == winrt::Windows::ApplicationModel::Activation::ActivationKind::Launch)
  {
    auto launchArgs = args.as<winrt::Windows::ApplicationModel::Activation::LaunchActivatedEventArgs>();
    winrt::hstring argHString = launchArgs.Arguments();

    plDynamicArray<const char*> argv;
    plCommandLineUtils::SplitCommandLineString(plStringUtf8(argHString.c_str()).GetData(), true, m_commandLineArgs, argv);

    m_application->SetCommandLineArguments(argv.GetCount(), argv.GetData());
  }
}

PLASMA_FOUNDATION_DLL plResult plUWPRun(plApplication* pApp)
{
  {
    auto application = winrt::make<plUwpApplication>(pApp);
    winrt::Windows::ApplicationModel::Core::CoreApplication::Run(application.as<winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>());
  }

  return PLASMA_SUCCESS;
}

#endif

PLASMA_STATICLINK_FILE(Foundation, Foundation_Application_Implementation_uwp_Application_uwp);
