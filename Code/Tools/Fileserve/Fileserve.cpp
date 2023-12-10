#include <Fileserve/FileservePCH.h>

#include <Fileserve/Fileserve.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#ifdef PLASMA_USE_QT
#  include <Fileserve/Gui.moc.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <QApplication>
#endif

#ifdef PLASMA_USE_QT
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
#else
int main(int argc, const char** argv)
{
#endif
  plFileserverApp* pApp = new plFileserverApp();

#ifdef PLASMA_USE_QT
  plCommandLineUtils::GetGlobalInstance()->SetCommandLine();

  int argc = 0;
  char** argv = nullptr;
  QApplication* pQtApplication = new QApplication(argc, const_cast<char**>(argv));
  pQtApplication->setApplicationName("plFileserve");
  pQtApplication->setOrganizationDomain("https://plasmagameengine.com/");
  pQtApplication->setOrganizationName("Plasma Engine Project");
  pQtApplication->setApplicationVersion("1.0.0");

  plRun_Startup(pApp).IgnoreResult();

  CreateFileserveMainWindow(pApp);
  pQtApplication->exec();
  plRun_Shutdown(pApp);
#else
  pApp->SetCommandLineArguments((plUInt32)argc, argv);
  plRun(pApp);
#endif

  const int iReturnCode = pApp->GetReturnCode();
  if (iReturnCode != 0)
  {

    std::string text = pApp->TranslateReturnCode();
    if (!text.empty())
      printf("Return Code: '%s'\n", text.c_str());
  }

  delete pApp;

  return iReturnCode;
}

plResult plFileserverApp::BeforeCoreSystemsStartup()
{
  plStartup::AddApplicationTag("tool");
  plStartup::AddApplicationTag("fileserve");

  return SUPER::BeforeCoreSystemsStartup();
}

void plFileserverApp::FileserverEventHandler(const plFileserverEvent& e)
{
  switch (e.m_Type)
  {
    case plFileserverEvent::Type::ClientConnected:
    case plFileserverEvent::Type::ClientReconnected:
      ++m_uiConnections;
      m_TimeTillClosing = plTime::MakeZero();
      break;
    case plFileserverEvent::Type::ClientDisconnected:
      --m_uiConnections;

      if (m_uiConnections == 0 && m_CloseAppTimeout.GetSeconds() > 0)
      {
        // reset the timer
        m_TimeTillClosing = plTime::Now() + m_CloseAppTimeout;
      }

      break;
    default:
      break;
  }
}
