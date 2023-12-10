#include <Editor/EditorPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
#  include <shellscalingapi.h>
#endif

class plEditorApplication : public plApplication
{
public:
  using SUPER = plApplication;

  plEditorApplication()
    : plApplication("PlasmaEditor")
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif
    EnableMemoryLeakReporting(true);

    m_pEditorApp = new plQtEditorApp;
  }

  virtual plResult BeforeCoreSystemsStartup() override
  {
    plStartup::AddApplicationTag("tool");
    plStartup::AddApplicationTag("editor");
    plStartup::AddApplicationTag("editorapp");

    plQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    return PLASMA_SUCCESS;
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    plQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  virtual Execution Run() override
  {
    {
      plStringBuilder cmdHelp;
      if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested, "_Editor;cvar"))
      {
        plQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        return plApplication::Execution::Quit;
      }
    }

    plQtEditorApp::GetSingleton()->StartupEditor();
    {
      const plInt32 iReturnCode = plQtEditorApp::GetSingleton()->RunEditor();
      SetReturnCode(iReturnCode);
    }
    plQtEditorApp::GetSingleton()->ShutdownEditor();

    return plApplication::Execution::Quit;
  }

private:
  plQtEditorApp* m_pEditorApp;
};

PLASMA_APPLICATION_ENTRY_POINT(plEditorApplication);
