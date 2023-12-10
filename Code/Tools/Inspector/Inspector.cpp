#include <Inspector/InspectorPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Communication/Telemetry.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>
#include <QApplication>
#include <QSettings>
#include <qstylefactory.h>

class plInspectorApp : public plApplication
{
public:
  using SUPER = plApplication;

  plInspectorApp()
    : plApplication("plasmaInspector")
  {
  }

  void SetStyleSheet()
  {
    plColorGammaUB highlightColor = plColorScheme::DarkUI(plColorScheme::PlasmaBranding);
    plColorGammaUB highlightColorDisabled = plColorScheme::DarkUI(plColorScheme::PlasmaBranding) * 0.5f;
    plColorGammaUB linkColor = plColorScheme::LightUI(plColorScheme::Blue);
    plColorGammaUB linkVisitedColor = plColorScheme::LightUI(plColorScheme::Orange);

    QApplication::setStyle(QStyleFactory::create("fusion"));
    QPalette palette;

    palette.setColor(QPalette::WindowText, QColor(221, 221, 221));
    palette.setColor(QPalette::Button, QColor(60, 60, 60));
    palette.setColor(QPalette::Light, QColor(62, 62, 62));
    palette.setColor(QPalette::Midlight, QColor(58, 58, 58));
    palette.setColor(QPalette::Dark, QColor(40, 40, 40));
    palette.setColor(QPalette::Mid, QColor(54, 54, 54));
    palette.setColor(QPalette::Text, QColor(221, 221, 221));
    palette.setColor(QPalette::BrightText, QColor(221, 221, 221));
    palette.setColor(QPalette::ButtonText, QColor(221, 221, 221));
    palette.setColor(QPalette::Base, QColor(25, 25, 25));
    palette.setColor(QPalette::Window, QColor(40, 40, 40));
    palette.setColor(QPalette::Shadow, QColor(0, 0, 0));
    palette.setColor(QPalette::Highlight, plToQtColor(highlightColor));
    palette.setColor(QPalette::HighlightedText, QColor(52, 52, 52));
    palette.setColor(QPalette::Link, plToQtColor(linkColor));
    palette.setColor(QPalette::LinkVisited, plToQtColor(linkVisitedColor));
    palette.setColor(QPalette::AlternateBase, QColor(37, 37, 40));
    QBrush NoRoleBrush(QColor(0, 0, 0), Qt::NoBrush);
    palette.setBrush(QPalette::NoRole, NoRoleBrush);
    palette.setColor(QPalette::ToolTipBase, QColor(52, 52, 52));
    palette.setColor(QPalette::ToolTipText, QColor(221, 221, 221));
    palette.setColor(QPalette::PlaceholderText, QColor(200, 200, 200).darker());

    palette.setColor(QPalette::Disabled, QPalette::Window, QColor(25, 25, 25));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(128, 128, 128));
    palette.setColor(QPalette::Disabled, QPalette::Button, QColor(35, 35, 35));
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(105, 105, 105));
    palette.setColor(QPalette::Disabled, QPalette::BrightText, QColor(255, 255, 255));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, plToQtColor(highlightColorDisabled));

    QApplication::setPalette(palette);
  }

  virtual plResult BeforeCoreSystemsStartup() override
  {
    plStartup::AddApplicationTag("tool");
    plStartup::AddApplicationTag("inspector");

    return plApplication::BeforeCoreSystemsStartup();
  }

  virtual Execution Run() override
  {
    int iArgs = GetArgumentCount();
    char** cArgs = (char**)GetArgumentsArray();

    QApplication app(iArgs, cArgs);
    QCoreApplication::setOrganizationDomain("https://plasmagameengine.com/");
    QCoreApplication::setOrganizationName("Plasma Engine Project");
    QCoreApplication::setApplicationName("Plasma Inspector");
    QCoreApplication::setApplicationVersion("1.0.0");

    SetStyleSheet();

    plQtMainWindow MainWindow;

    plTelemetry::AcceptMessagesForSystem('CVAR', true, plQtCVarsWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('CMD', true, plQtCVarsWidget::ProcessTelemetryConsole, nullptr);
    plTelemetry::AcceptMessagesForSystem(' LOG', true, plQtLogDockWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem(' MEM', true, plQtMemoryWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('TIME', true, plQtTimeWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem(' APP', true, plQtMainWindow::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('FILE', true, plQtFileWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('INPT', true, plQtInputWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('STRT', true, plQtSubsystemsWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('STAT', true, plQtMainWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('PLUG', true, plQtPluginsWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('EVNT', true, plQtGlobalEventsWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('RFLC', true, plQtReflectionWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('TRAN', true, plQtDataWidget::ProcessTelemetry, nullptr);
    plTelemetry::AcceptMessagesForSystem('RESM', true, plQtResourceWidget::ProcessTelemetry, nullptr);

    QSettings Settings;
    const QString sServer = Settings.value("LastConnection", QLatin1String("localhost:1040")).toString();

    plTelemetry::ConnectToServer(sServer.toUtf8().data()).IgnoreResult();

    MainWindow.show();
    SetReturnCode(app.exec());

    plTelemetry::CloseConnection();

    return plApplication::Execution::Quit;
  }
};

PLASMA_APPLICATION_ENTRY_POINT(plInspectorApp);
