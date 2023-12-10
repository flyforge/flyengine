#include <Inspector/InspectorPCH.h>

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

const int g_iDockingStateVersion = 1;

plQtMainWindow* plQtMainWindow::s_pWidget = nullptr;

plQtMainWindow::plQtMainWindow()
  : QMainWindow()
{
  s_pWidget = this;

  setupUi(this);

  m_DockManager = new ads::CDockManager(this);
  m_DockManager->setConfigFlags(
    static_cast<ads::CDockManager::ConfigFlags>(ads::CDockManager::DockAreaHasCloseButton | ads::CDockManager::DockAreaCloseButtonClosesTab |
                                                ads::CDockManager::OpaqueSplitterResize | ads::CDockManager::AllTabsHaveCloseButton));

  QSettings Settings;
  SetAlwaysOnTop((OnTopMode)Settings.value("AlwaysOnTop", (int)WhenConnected).toInt());

  Settings.beginGroup("MainWindow");

  const bool bRestoreDockingState = Settings.value("DockingVersion") == g_iDockingStateVersion;

  if (bRestoreDockingState)
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
  }

  // The dock manager will set ownership to null on add so there is no reason to provide an owner here.
  // Setting one will actually cause memory corruptions on shutdown for unknown reasons.
  plQtMainWidget* pMainWidget = new plQtMainWidget();
  plQtLogDockWidget* pLogWidget = new plQtLogDockWidget();
  plQtMemoryWidget* pMemoryWidget = new plQtMemoryWidget();
  plQtTimeWidget* pTimeWidget = new plQtTimeWidget();
  plQtInputWidget* pInputWidget = new plQtInputWidget();
  plQtCVarsWidget* pCVarsWidget = new plQtCVarsWidget();
  plQtSubsystemsWidget* pSubsystemsWidget = new plQtSubsystemsWidget();
  plQtFileWidget* pFileWidget = new plQtFileWidget();
  plQtPluginsWidget* pPluginsWidget = new plQtPluginsWidget();
  plQtGlobalEventsWidget* pGlobalEventesWidget = new plQtGlobalEventsWidget();
  plQtReflectionWidget* pReflectionWidget = new plQtReflectionWidget();
  plQtDataWidget* pDataWidget = new plQtDataWidget();
  plQtResourceWidget* pResourceWidget = new plQtResourceWidget();

  PLASMA_VERIFY(nullptr != QWidget::connect(pMainWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pLogWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pTimeWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pMemoryWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pInputWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pCVarsWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pReflectionWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pSubsystemsWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pFileWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pPluginsWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(
    nullptr != QWidget::connect(pGlobalEventesWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pDataWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");
  PLASMA_VERIFY(nullptr != QWidget::connect(pResourceWidget, &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");

  QMenu* pHistoryMenu = new QMenu;
  pHistoryMenu->setTearOffEnabled(true);
  pHistoryMenu->setTitle(QLatin1String("Stat Histories"));
  pHistoryMenu->setIcon(QIcon(":/Icons/Icons/StatHistory.svg"));

  for (plUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i] = new plQtStatVisWidget(this, i);
    m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, m_pStatHistoryWidgets[i]);

    PLASMA_VERIFY(
      nullptr != QWidget::connect(m_pStatHistoryWidgets[i], &ads::CDockWidget::viewToggled, this, &plQtMainWindow::DockWidgetVisibilityChanged), "");

    pHistoryMenu->addAction(&m_pStatHistoryWidgets[i]->m_ShowWindowAction);

    m_pActionShowStatIn[i] = new QAction(this);

    PLASMA_VERIFY(nullptr != QWidget::connect(m_pActionShowStatIn[i], &QAction::triggered, plQtMainWidget::s_pWidget, &plQtMainWidget::ShowStatIn), "");
  }

  // delay this until after all widgets are created
  for (plUInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->toggleView(false); // hide
  }

  setContextMenuPolicy(Qt::NoContextMenu);

  menuWindows->addMenu(pHistoryMenu);

  pMemoryWidget->raise();

  m_DockManager->addDockWidget(ads::LeftDockWidgetArea, pMainWidget);
  m_DockManager->addDockWidget(ads::CenterDockWidgetArea, pLogWidget);

  m_DockManager->addDockWidget(ads::RightDockWidgetArea, pCVarsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pGlobalEventesWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pDataWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pInputWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pPluginsWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pReflectionWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pResourceWidget);
  m_DockManager->addDockWidgetTab(ads::RightDockWidgetArea, pSubsystemsWidget);

  m_DockManager->addDockWidget(ads::BottomDockWidgetArea, pFileWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pMemoryWidget);
  m_DockManager->addDockWidgetTab(ads::BottomDockWidgetArea, pTimeWidget);


  pLogWidget->raise();
  pCVarsWidget->raise();

  if (bRestoreDockingState)
  {
    auto dockState = Settings.value("DockManagerState");
    if (dockState.isValid() && dockState.typeId() == QMetaType::QByteArray)
    {
      m_DockManager->restoreState(dockState.toByteArray(), 1);
    }

    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());

    if (Settings.value("IsMaximized", isMaximized()).toBool())
    {
      showMaximized();
    }

    restoreState(Settings.value("WindowState", saveState()).toByteArray());
  }

  Settings.endGroup();

  for (plInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->Load();

  SetupNetworkTimer();
}

plQtMainWindow::~plQtMainWindow()
{
  for (plInt32 i = 0; i < 10; ++i)
  {
    m_pStatHistoryWidgets[i]->Save();
  }
  // The dock manager does not take ownership of dock widgets.
  auto dockWidgets = m_DockManager->dockWidgetsMap();
  for (auto it = dockWidgets.begin(); it != dockWidgets.end(); ++it)
  {
    m_DockManager->removeDockWidget(it.value());
    delete it.value();
  }
}

void plQtMainWindow::closeEvent(QCloseEvent* pEvent)
{
  const bool bMaximized = isMaximized();
  if (bMaximized)
    showNormal();

  QSettings Settings;

  Settings.beginGroup("MainWindow");

  Settings.setValue("DockingVersion", g_iDockingStateVersion);
  Settings.setValue("DockManagerState", m_DockManager->saveState(1));
  Settings.setValue("WindowGeometry", saveGeometry());
  Settings.setValue("WindowState", saveState());
  Settings.setValue("IsMaximized", bMaximized);
  Settings.setValue("WindowPosition", pos());
  if (!bMaximized)
    Settings.setValue("WindowSize", size());

  Settings.endGroup();
}

void plQtMainWindow::SetupNetworkTimer()
{
  // reset the timer to fire again
  if (m_pNetworkTimer == nullptr)
    m_pNetworkTimer = new QTimer(this);

  m_pNetworkTimer->singleShot(40, this, SLOT(UpdateNetworkTimeOut()));
}

void plQtMainWindow::UpdateNetworkTimeOut()
{
  UpdateNetwork();

  SetupNetworkTimer();
}

void plQtMainWindow::UpdateNetwork()
{
  bool bResetStats = false;

  {
    static plUInt32 uiServerID = 0;
    static bool bConnected = false;
    static plString sLastServerName;

    if (plTelemetry::IsConnectedToServer())
    {
      if (uiServerID != plTelemetry::GetServerID())
      {
        uiServerID = plTelemetry::GetServerID();
        bResetStats = true;

        plStringBuilder s;
        s.Format("Connected to new Server with ID {0}", uiServerID);

        plQtLogDockWidget::s_pWidget->Log(s.GetData());
      }
      else if (!bConnected)
      {
        plQtLogDockWidget::s_pWidget->Log("Reconnected to Server.");
      }

      if (sLastServerName != plTelemetry::GetServerName())
      {
        sLastServerName = plTelemetry::GetServerName();
        setWindowTitle(QString("plInspector - %1").arg(sLastServerName.GetData()));
      }

      bConnected = true;
    }
    else
    {
      if (bConnected)
      {
        plQtLogDockWidget::s_pWidget->Log("Lost Connection to Server.");
        setWindowTitle(QString("plInspector - disconnected"));
        sLastServerName.Clear();
      }

      bConnected = false;
    }
  }

  if (bResetStats)
  {


    plQtMainWidget::s_pWidget->ResetStats();
    plQtLogDockWidget::s_pWidget->ResetStats();
    plQtMemoryWidget::s_pWidget->ResetStats();
    plQtTimeWidget::s_pWidget->ResetStats();
    plQtInputWidget::s_pWidget->ResetStats();
    plQtCVarsWidget::s_pWidget->ResetStats();
    plQtReflectionWidget::s_pWidget->ResetStats();
    plQtFileWidget::s_pWidget->ResetStats();
    plQtPluginsWidget::s_pWidget->ResetStats();
    plQtSubsystemsWidget::s_pWidget->ResetStats();
    plQtGlobalEventsWidget::s_pWidget->ResetStats();
    plQtDataWidget::s_pWidget->ResetStats();
    plQtResourceWidget::s_pWidget->ResetStats();
  }

  UpdateAlwaysOnTop();

  plQtMainWidget::s_pWidget->UpdateStats();
  plQtPluginsWidget::s_pWidget->UpdateStats();
  plQtSubsystemsWidget::s_pWidget->UpdateStats();
  plQtMemoryWidget::s_pWidget->UpdateStats();
  plQtTimeWidget::s_pWidget->UpdateStats();
  plQtFileWidget::s_pWidget->UpdateStats();
  plQtResourceWidget::s_pWidget->UpdateStats();
  // plQtDataWidget::s_pWidget->UpdateStats();

  for (plInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->UpdateStats();

  plTelemetry::PerFrameUpdate();
}

void plQtMainWindow::DockWidgetVisibilityChanged(bool bVisible)
{
  // TODO: add menu entry for qt main widget

  ActionShowWindowLog->setChecked(!plQtLogDockWidget::s_pWidget->isClosed());
  ActionShowWindowMemory->setChecked(!plQtMemoryWidget::s_pWidget->isClosed());
  ActionShowWindowTime->setChecked(!plQtTimeWidget::s_pWidget->isClosed());
  ActionShowWindowInput->setChecked(!plQtInputWidget::s_pWidget->isClosed());
  ActionShowWindowCVar->setChecked(!plQtCVarsWidget::s_pWidget->isClosed());
  ActionShowWindowReflection->setChecked(!plQtReflectionWidget::s_pWidget->isClosed());
  ActionShowWindowSubsystems->setChecked(!plQtSubsystemsWidget::s_pWidget->isClosed());
  ActionShowWindowFile->setChecked(!plQtFileWidget::s_pWidget->isClosed());
  ActionShowWindowPlugins->setChecked(!plQtPluginsWidget::s_pWidget->isClosed());
  ActionShowWindowGlobalEvents->setChecked(!plQtGlobalEventsWidget::s_pWidget->isClosed());
  ActionShowWindowData->setChecked(!plQtDataWidget::s_pWidget->isClosed());
  ActionShowWindowResource->setChecked(!plQtResourceWidget::s_pWidget->isClosed());

  for (plInt32 i = 0; i < 10; ++i)
    m_pStatHistoryWidgets[i]->m_ShowWindowAction.setChecked(!m_pStatHistoryWidgets[i]->isClosed());
}


void plQtMainWindow::SetAlwaysOnTop(OnTopMode Mode)
{
  m_OnTopMode = Mode;

  QSettings Settings;
  Settings.setValue("AlwaysOnTop", (int)m_OnTopMode);

  ActionNeverOnTop->setChecked((m_OnTopMode == Never) ? Qt::Checked : Qt::Unchecked);
  ActionAlwaysOnTop->setChecked((m_OnTopMode == Always) ? Qt::Checked : Qt::Unchecked);
  ActionOnTopWhenConnected->setChecked((m_OnTopMode == WhenConnected) ? Qt::Checked : Qt::Unchecked);

  UpdateAlwaysOnTop();
}

void plQtMainWindow::UpdateAlwaysOnTop()
{
  static bool bOnTop = false;

  bool bNewState = bOnTop;
  PLASMA_IGNORE_UNUSED(bNewState);

  if (m_OnTopMode == Always || (m_OnTopMode == WhenConnected && plTelemetry::IsConnectedToServer()))
    bNewState = true;
  else
    bNewState = false;

  if (bOnTop != bNewState)
  {
    bOnTop = bNewState;

    hide();

    if (bOnTop)
      setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
    else
      setWindowFlags(windowFlags() & ~Qt::WindowStaysOnTopHint | Qt::WindowStaysOnBottomHint);

    show();
  }
}

void plQtMainWindow::ProcessTelemetry(void* pUnuseed)
{
  if (!s_pWidget)
    return;

  plTelemetryMessage Msg;

  while (plTelemetry::RetrieveMessage(' APP', Msg) == PLASMA_SUCCESS)
  {
    switch (Msg.GetMessageID())
    {
      case 'ASRT':
      {
        plString sSourceFile, sFunction, sExpression, sMessage;
        plUInt32 uiLine = 0;

        Msg.GetReader() >> sSourceFile;
        Msg.GetReader() >> uiLine;
        Msg.GetReader() >> sFunction;
        Msg.GetReader() >> sExpression;
        Msg.GetReader() >> sMessage;

        plQtLogDockWidget::s_pWidget->Log("");
        plQtLogDockWidget::s_pWidget->Log("<<< Application Assertion >>>");
        plQtLogDockWidget::s_pWidget->Log("");

        plQtLogDockWidget::s_pWidget->Log(plFmt("    Expression: '{0}'", sExpression));
        plQtLogDockWidget::s_pWidget->Log("");

        plQtLogDockWidget::s_pWidget->Log(plFmt("    Message: '{0}'", sMessage));
        plQtLogDockWidget::s_pWidget->Log("");

        plQtLogDockWidget::s_pWidget->Log(plFmt("   File: '{0}'", sSourceFile));

        plQtLogDockWidget::s_pWidget->Log(plFmt("   Line: {0}", uiLine));

        plQtLogDockWidget::s_pWidget->Log(plFmt("   In Function: '{0}'", sFunction));

        plQtLogDockWidget::s_pWidget->Log("");

        plQtLogDockWidget::s_pWidget->Log(">>> Application Assertion <<<");
        plQtLogDockWidget::s_pWidget->Log("");
      }
      break;
    }
  }
}
