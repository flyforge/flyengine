#include <TestFramework/TestFrameworkPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <Foundation/Strings/StringConversion.h>
#  include <TestFramework/Framework/uwp/uwpTestApplication.h>
#  include <TestFramework/Framework/uwp/uwpTestFramework.h>
#  include <windows.ui.core.h>
#  include <wrl/event.h>

using namespace ABI::Windows::Foundation;

plUwpTestApplication::plUwpTestApplication(plTestFramework& testFramework)
  : m_testFramework(testFramework)
{
}

plUwpTestApplication::~plUwpTestApplication() {}

HRESULT plUwpTestApplication::CreateView(IFrameworkView** viewProvider)
{
  *viewProvider = this;
  return S_OK;
}

HRESULT plUwpTestApplication::Initialize(ICoreApplicationView* applicationView)
{
  using OnActivatedHandler =
    __FITypedEventHandler_2_Windows__CApplicationModel__CCore__CCoreApplicationView_Windows__CApplicationModel__CActivation__CIActivatedEventArgs;
  PLASMA_SUCCEED_OR_RETURN(
    applicationView->add_Activated(Callback<OnActivatedHandler>(this, &plUwpTestApplication::OnActivated).Get(), &m_eventRegistrationOnActivate));



  plStartup::StartupBaseSystems();

  return S_OK;
}

HRESULT plUwpTestApplication::SetWindow(ABI::Windows::UI::Core::ICoreWindow* window)
{
  return S_OK;
}

HRESULT plUwpTestApplication::Load(HSTRING entryPoint)
{
  return S_OK;
}

HRESULT plUwpTestApplication::Run()
{
  ComPtr<ABI::Windows::UI::Core::ICoreWindowStatic> coreWindowStatics;
  PLASMA_SUCCEED_OR_RETURN(
    ABI::Windows::Foundation::GetActivationFactory(HStringReference(RuntimeClass_Windows_UI_Core_CoreWindow).Get(), &coreWindowStatics));
  ComPtr<ABI::Windows::UI::Core::ICoreWindow> coreWindow;
  PLASMA_SUCCEED_OR_RETURN(coreWindowStatics->GetForCurrentThread(&coreWindow));
  ComPtr<ABI::Windows::UI::Core::ICoreDispatcher> dispatcher;
  PLASMA_SUCCEED_OR_RETURN(coreWindow->get_Dispatcher(&dispatcher));

  while (m_testFramework.RunTestExecutionLoop() == plTestAppRun::Continue)
  {
    dispatcher->ProcessEvents(ABI::Windows::UI::Core::CoreProcessEventsOption_ProcessAllIfPresent);
  }

  return S_OK;
}

HRESULT plUwpTestApplication::Uninitialize()
{
  m_testFramework.AbortTests();
  return S_OK;
}

HRESULT plUwpTestApplication::OnActivated(ICoreApplicationView* applicationView, IActivatedEventArgs* args)
{
  applicationView->remove_Activated(m_eventRegistrationOnActivate);

  ActivationKind activationKind;
  PLASMA_SUCCEED_OR_RETURN(args->get_Kind(&activationKind));

  if (activationKind == ActivationKind_Launch)
  {
    ComPtr<ILaunchActivatedEventArgs> launchArgs;
    PLASMA_SUCCEED_OR_RETURN(args->QueryInterface(launchArgs.GetAddressOf()));

    HString argHString;
    PLASMA_SUCCEED_OR_RETURN(launchArgs->get_Arguments(argHString.GetAddressOf()));

    plDynamicArray<plString> commandLineArgs;
    plDynamicArray<const char*> argv;
    plCommandLineUtils::SplitCommandLineString(plStringUtf8(argHString).GetData(), true, commandLineArgs, argv);

    plCommandLineUtils cmd;
    cmd.SetCommandLine(argv.GetCount(), argv.GetData(), plCommandLineUtils::PreferOsArgs);

    m_testFramework.GetTestSettingsFromCommandLine(cmd);

    // Setup an extended execution session to prevent app from going to sleep during testing.
    plUwpUtils::CreateInstance<IExtendedExecutionSession>(
      RuntimeClass_Windows_ApplicationModel_ExtendedExecution_ExtendedExecutionSession, m_extendedExecutionSession);
    PLASMA_ASSERT_DEV(m_extendedExecutionSession, "Failed to create extended session. Can't prevent app from backgrounding during testing.");
    m_extendedExecutionSession->put_Reason(ExtendedExecutionReason::ExtendedExecutionReason_Unspecified);
    plStringHString desc("Keep Unit Tests Running");
    m_extendedExecutionSession->put_Description(desc.GetData().Get());

    using OnRevokedHandler = __FITypedEventHandler_2_IInspectable_Windows__CApplicationModel__CExtendedExecution__CExtendedExecutionRevokedEventArgs;
    PLASMA_SUCCEED_OR_RETURN(m_extendedExecutionSession->add_Revoked(
      Callback<OnRevokedHandler>(this, &plUwpTestApplication::OnSessionRevoked).Get(), &m_eventRegistrationOnRevokedSession));

    ComPtr<__FIAsyncOperation_1_Windows__CApplicationModel__CExtendedExecution__CExtendedExecutionResult> pAsyncOp;
    if (SUCCEEDED(m_extendedExecutionSession->RequestExtensionAsync(&pAsyncOp)))
    {
      plUwpUtils::plWinRtPutCompleted<ExtendedExecutionResult, ExtendedExecutionResult>(pAsyncOp, [this](const ExtendedExecutionResult& pResult) {
        switch (pResult)
        {
          case ExtendedExecutionResult::ExtendedExecutionResult_Allowed:
            plLog::Info("Extended session is active.");
            break;
          case ExtendedExecutionResult::ExtendedExecutionResult_Denied:
            plLog::Error("Extended session is denied.");
            break;
        }
      });
    }
  }

  return S_OK;
}

HRESULT plUwpTestApplication::OnSessionRevoked(IInspectable* sender, IExtendedExecutionRevokedEventArgs* args)
{
  plLog::Error("Extended session revoked.");
  return S_OK;
}

#endif

PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_uwp_uwpTestApplication);
