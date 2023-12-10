#pragma once

#include <Foundation/FoundationInternal.h>
PLASMA_FOUNDATION_INTERNAL_HEADER

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)

#  include <Foundation/Basics.h>
#  include <Foundation/Strings/String.h>

#  include <Foundation/Basics/Platform/uwp/UWPUtils.h>
#  include <winrt/base.h>

#  include <winrt/Windows.ApplicationModel.Activation.h>
#  include <winrt/Windows.ApplicationModel.Core.h>

class plApplication;

/// Minimal implementation of a uwp application.
class plUwpApplication : public winrt::implements<plUwpApplication, winrt::Windows::ApplicationModel::Core::IFrameworkView, winrt::Windows::ApplicationModel::Core::IFrameworkViewSource>
{
public:
  plUwpApplication(plApplication* application);
  virtual ~plUwpApplication();

  // Inherited via IFrameworkViewSource
  winrt::Windows::ApplicationModel::Core::IFrameworkView CreateView();

  // Inherited via IFrameworkView
  void Initialize(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& applicationView);
  void SetWindow(winrt::Windows::UI::Core::CoreWindow const& window);
  void Load(winrt::hstring const& entryPoint);
  void Run();
  void Uninitialize();

private:
  // Application lifecycle event handlers.
  void OnViewActivated(winrt::Windows::ApplicationModel::Core::CoreApplicationView const& sender, winrt::Windows::ApplicationModel::Activation::IActivatedEventArgs const& args);

  winrt::event_token m_activateRegistrationToken;

  plApplication* m_application;
  plDynamicArray<plString> m_commandLineArgs;
};

#endif
