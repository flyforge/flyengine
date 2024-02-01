#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDockWidget>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTimer>
#include <ToolsFoundation/Document/Document.h>
#include <ads/DockWidget.h>

plEvent<const plQtDocumentWindowEvent&> plQtDocumentWindow::s_Events;
plDynamicArray<plQtDocumentWindow*> plQtDocumentWindow::s_AllDocumentWindows;
bool plQtDocumentWindow::s_bAllowRestoreWindowLayout = true;

void plQtDocumentWindow::Constructor()
{
  s_AllDocumentWindows.PushBack(this);

  // status bar
  {
    connect(statusBar(), &QStatusBar::messageChanged, this, &plQtDocumentWindow::OnStatusBarMessageChanged);

    m_pPermanentDocumentStatusText = new QLabel();
    statusBar()->addWidget(m_pPermanentDocumentStatusText, 1);

    m_pPermanentGlobalStatusButton = new QToolButton();
    m_pPermanentGlobalStatusButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    m_pPermanentGlobalStatusButton->setVisible(false);
    statusBar()->addPermanentWidget(m_pPermanentGlobalStatusButton, 0);

    PL_VERIFY(connect(m_pPermanentGlobalStatusButton, &QToolButton::clicked, this, &plQtDocumentWindow::OnPermanentGlobalStatusClicked), "");
  }

  setDockNestingEnabled(true);

  plQtMenuBarActionMapView* pMenuBar = new plQtMenuBarActionMapView(this);
  setMenuBar(pMenuBar);

  plToolsProject::SuggestContainerWindow(m_pDocument);
  plQtContainerWindow* pContainer = plQtContainerWindow::GetContainerWindow();
  pContainer->AddDocumentWindow(this);

  plQtUiServices::s_Events.AddEventHandler(plMakeDelegate(&plQtDocumentWindow::UIServicesEventHandler, this));
  plQtUiServices::s_TickEvent.AddEventHandler(plMakeDelegate(&plQtDocumentWindow::UIServicesTickEventHandler, this));
}

plQtDocumentWindow::plQtDocumentWindow(plDocument* pDocument)
{
  m_pDocument = pDocument;
  m_sUniqueName = m_pDocument->GetDocumentPath();
  setObjectName(GetUniqueName());

  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plQtDocumentWindow::DocumentManagerEventHandler, this));
  pDocument->m_EventsOne.AddEventHandler(plMakeDelegate(&plQtDocumentWindow::DocumentEventHandler, this));

  Constructor();
}

plQtDocumentWindow::plQtDocumentWindow(const char* szUniqueName)
{
  m_pDocument = nullptr;
  m_sUniqueName = szUniqueName;
  setObjectName(GetUniqueName());

  Constructor();
}


plQtDocumentWindow::~plQtDocumentWindow()
{
  plQtUiServices::s_Events.RemoveEventHandler(plMakeDelegate(&plQtDocumentWindow::UIServicesEventHandler, this));
  plQtUiServices::s_TickEvent.RemoveEventHandler(plMakeDelegate(&plQtDocumentWindow::UIServicesTickEventHandler, this));

  s_AllDocumentWindows.RemoveAndSwap(this);

  if (m_pDocument)
  {
    m_pDocument->m_EventsOne.RemoveEventHandler(plMakeDelegate(&plQtDocumentWindow::DocumentEventHandler, this));
    plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plQtDocumentWindow::DocumentManagerEventHandler, this));
  }
}

void plQtDocumentWindow::SetVisibleInContainer(bool bVisible)
{
  if (m_bIsVisibleInContainer == bVisible)
    return;

  m_bIsVisibleInContainer = bVisible;
  InternalVisibleInContainerChanged(bVisible);

  if (m_bIsVisibleInContainer)
  {
    // if the window is now visible, immediately do a redraw and trigger the timers
    SlotRedraw();
    // Make sure the window gains focus as well when it becomes visible so that shortcuts will immediately work.
    setFocus();
  }
}

void plQtDocumentWindow::SetTargetFramerate(plInt16 iTargetFPS)
{
  if (m_iTargetFramerate == iTargetFPS)
    return;

  m_iTargetFramerate = iTargetFPS;

  if (m_iTargetFramerate != 0)
    SlotRedraw();
}

void plQtDocumentWindow::TriggerRedraw()
{
  SlotRedraw();
}

void plQtDocumentWindow::UIServicesTickEventHandler(const plQtUiServices::TickEvent& e)
{
  if (e.m_Type == plQtUiServices::TickEvent::Type::StartFrame && m_bIsVisibleInContainer)
  {
    const plInt32 iSystemFramerate = static_cast<plInt32>(plMath::Round(e.m_fRefreshRate));

    plInt32 iTargetFramerate = m_iTargetFramerate;
    if (iTargetFramerate <= 0)
      iTargetFramerate = iSystemFramerate;

    // if the application does not have focus, drastically reduce the update rate to limit CPU draw etc.
    if (QApplication::activeWindow() == nullptr)
      iTargetFramerate = plMath::Max(10, iTargetFramerate / 4);

    // We do not hit the requested framerate directly if the system framerate can't be evenly divided. We will chose the next higher framerate.
    if (iTargetFramerate < iSystemFramerate)
    {
      plUInt32 mod = plMath::Max(1u, (plUInt32)plMath::Floor(iSystemFramerate / (double)iTargetFramerate));
      if ((e.m_uiFrame % mod) != 0)
        return;
    }

    SlotRedraw();
  }
}


void plQtDocumentWindow::SlotRedraw()
{
  plStringBuilder sFilename = plPathUtils::GetFileName(this->GetUniqueName());
  PL_PROFILE_SCOPE(sFilename.GetData());
  {
    plQtDocumentWindowEvent e;
    e.m_Type = plQtDocumentWindowEvent::Type::BeforeRedraw;
    e.m_pWindow = this;
    s_Events.Broadcast(e, 1);
  }

  // if our window is not visible, interrupt the redrawing, and do nothing
  if (!m_bIsVisibleInContainer)
    return;

  m_bIsDrawingATM = true;
  InternalRedraw();
  m_bIsDrawingATM = false;
}

void plQtDocumentWindow::DocumentEventHandler(const plDocumentEvent& e)
{
  switch (e.m_Type)
  {
    case plDocumentEvent::Type::DocumentRenamed:
    {
      m_sUniqueName = m_pDocument->GetDocumentPath();
      setObjectName(GetUniqueName());
      plQtContainerWindow* pContainer = plQtContainerWindow::GetContainerWindow();
      pContainer->DocumentWindowRenamed(this);

      [[fallthrough]];
    }
    case plDocumentEvent::Type::ModifiedChanged:
    {
      plQtDocumentWindowEvent dwe;
      dwe.m_pWindow = this;
      dwe.m_Type = plQtDocumentWindowEvent::Type::WindowDecorationChanged;
      s_Events.Broadcast(dwe);
    }
    break;

    case plDocumentEvent::Type::EnsureVisible:
    {
      EnsureVisible();
    }
    break;

    case plDocumentEvent::Type::DocumentStatusMsg:
    {
      ShowTemporaryStatusBarMsg(e.m_sStatusMsg);
    }
    break;

    default:
      break;
  }
}

void plQtDocumentWindow::DocumentManagerEventHandler(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentClosing:
    {
      if (e.m_pDocument == m_pDocument)
      {
        ShutdownDocumentWindow();
        return;
      }
    }
    break;

    default:
      break;
  }
}

void plQtDocumentWindow::UIServicesEventHandler(const plQtUiServices::Event& e)
{
  switch (e.m_Type)
  {
    case plQtUiServices::Event::Type::ShowDocumentTemporaryStatusBarText:
      ShowTemporaryStatusBarMsg(plFmt(e.m_sText), e.m_Time);
      break;

    case plQtUiServices::Event::Type::ShowDocumentPermanentStatusBarText:
    {
      if (m_pPermanentGlobalStatusButton)
      {
        QPalette pal = palette();

        switch (e.m_TextType)
        {
          case plQtUiServices::Event::Info:
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Log.svg"));
            break;

          case plQtUiServices::Event::Warning:
            pal.setColor(QPalette::WindowText, QColor(255, 100, 0));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Warning.svg"));
            break;

          case plQtUiServices::Event::Error:
            pal.setColor(QPalette::WindowText, QColor(Qt::red));
            m_pPermanentGlobalStatusButton->setIcon(QIcon(":/GuiFoundation/Icons/Error.svg"));
            break;
        }

        m_pPermanentGlobalStatusButton->setPalette(pal);
        m_pPermanentGlobalStatusButton->setText(QString::fromUtf8(e.m_sText, e.m_sText.GetElementCount()));
        m_pPermanentGlobalStatusButton->setVisible(!m_pPermanentGlobalStatusButton->text().isEmpty());
      }
    }
    break;

    default:
      break;
  }
}

plString plQtDocumentWindow::GetDisplayNameShort() const
{
  plStringBuilder s = GetDisplayName();
  s = s.GetFileName();

  if (m_pDocument && m_pDocument->IsModified())
    s.Append('*');

  return s;
}

void plQtDocumentWindow::showEvent(QShowEvent* event)
{
  QMainWindow::showEvent(event);
  SetVisibleInContainer(true);
}

void plQtDocumentWindow::hideEvent(QHideEvent* event)
{
  QMainWindow::hideEvent(event);
  SetVisibleInContainer(false);
}

bool plQtDocumentWindow::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::ShortcutOverride || e->type() == QEvent::KeyPress)
  {
    // This filter is added by plQtContainerWindow::AddDocumentWindow as that ones is the ony code path that can connect dock container to their content.
    // This filter is necessary as clicking any action in a menu bar sets the focus to the parent CDockWidget at which point further shortcuts would stop working.
    if (qobject_cast<ads::CDockWidget*>(obj))
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
      if (plQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, e->type() == QEvent::ShortcutOverride))
        return true;
    }

    // Some central widgets consume the shortcut (or any key press for that matter) instead of passing it up the parent hierarchy. For example a QGraphicsView will forward any key-press to the QGraphicsScene which will consume every event. As a workaround, we overrule the central widget by default when it comes to shortcuts.
    if (obj == centralWidget())
    {
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
      if (plQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, e->type() == QEvent::ShortcutOverride))
        return true;
    }
  }

  return false;
}

bool plQtDocumentWindow::event(QEvent* event)
{
  if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)
  {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
    if (plQtProxy::TriggerDocumentAction(m_pDocument, keyEvent, event->type() == QEvent::ShortcutOverride))
      return true;
  }
  return QMainWindow::event(event);
}

void plQtDocumentWindow::FinishWindowCreation()
{
  if (centralWidget())
    centralWidget()->installEventFilter(this);

  ScheduleRestoreWindowLayout();
}

void plQtDocumentWindow::ScheduleRestoreWindowLayout()
{
  QTimer::singleShot(0, this, SLOT(SlotRestoreLayout()));
}

void plQtDocumentWindow::SlotRestoreLayout()
{
  RestoreWindowLayout();
}

void plQtDocumentWindow::SaveWindowLayout()
{
  // This is a workaround for newer Qt versions (5.13 or so) that seem to change the state of QDockWidgets to "closed" once the parent
  // QMainWindow gets the closeEvent, even though they still exist and the QMainWindow is not yet deleted. Previously this function was
  // called multiple times, including once after the QMainWindow got its closeEvent, which would then save a corrupted state. Therefore,
  // once the parent plQtContainerWindow gets the closeEvent, we now prevent further saving of the window layout.
  if (!m_bAllowSaveWindowLayout)
    return;

  const bool bMaximized = isMaximized();

  if (bMaximized)
    showNormal();

  plStringBuilder sGroup;
  sGroup.SetFormat("DocumentWnd_{0}", GetWindowLayoutGroupName());

  QSettings Settings;
  Settings.beginGroup(QString::fromUtf8(sGroup, sGroup.GetElementCount()));
  {
    // All other properties are defined by the outer container window.
    Settings.setValue("WindowState", saveState());
  }
  Settings.endGroup();
}

void plQtDocumentWindow::RestoreWindowLayout()
{
  if (!s_bAllowRestoreWindowLayout)
    return;

  plQtScopedUpdatesDisabled _(this);

  plStringBuilder sGroup;
  sGroup.SetFormat("DocumentWnd_{0}", GetWindowLayoutGroupName());

  {
    QSettings Settings;
    Settings.beginGroup(QString::fromUtf8(sGroup, sGroup.GetElementCount()));
    {
      restoreState(Settings.value("WindowState", saveState()).toByteArray());
    }
    Settings.endGroup();

    // with certain Qt versions the window state could be saved corrupted
    // if that is the case, make sure that non-closable widgets get restored to be visible
    // otherwise the user would need to delete the serialized state from the registry
    {
      for (QDockWidget* dockWidget : findChildren<QDockWidget*>())
      {
        // not closable means the user can generally not change the visible state -> make sure it is visible
        if (!dockWidget->features().testFlag(QDockWidget::DockWidgetClosable) && dockWidget->isHidden())
        {
          dockWidget->show();
        }
      }
    }
  }

  statusBar()->clearMessage();
}

void plQtDocumentWindow::DisableWindowLayoutSaving()
{
  m_bAllowSaveWindowLayout = false;
}

plStatus plQtDocumentWindow::SaveDocument()
{
  if (m_pDocument)
  {
    {
      if (m_pDocument->GetUnknownObjectTypeInstances() > 0)
      {
        if (plQtUiServices::MessageBoxQuestion("Warning! This document contained unknown object types that could not be loaded. Saving the "
                                               "document means those objects will get lost permanently.\n\nDo you really want to save this "
                                               "document?",
              QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No) != QMessageBox::StandardButton::Yes)
          return plStatus(PL_SUCCESS); // failed successfully
      }
    }

    plStatus res = m_pDocument->SaveDocument();

    plStringBuilder s, s2;
    s.SetFormat("Failed to save document:\n'{0}'", m_pDocument->GetDocumentPath());
    s2.SetFormat("Successfully saved document:\n'{0}'", m_pDocument->GetDocumentPath());

    plQtUiServices::MessageBoxStatus(res, s, s2);

    if (res.m_Result.Failed())
    {
      ShowTemporaryStatusBarMsg("Failed to save document");
      return res;
    }

    ShowTemporaryStatusBarMsg("Document saved");
  }

  return plStatus(PL_SUCCESS);
}

void plQtDocumentWindow::ShowTemporaryStatusBarMsg(const plFormatString& msg, plTime duration)
{
  plStringBuilder tmp;
  statusBar()->showMessage(QString::fromUtf8(msg.GetTextCStr(tmp)), (int)duration.GetMilliseconds());
}


void plQtDocumentWindow::SetPermanentStatusBarMsg(const plFormatString& text)
{
  if (!text.IsEmpty())
  {
    // clear temporary message
    statusBar()->clearMessage();
  }

  plStringBuilder tmp;
  m_pPermanentDocumentStatusText->setText(QString::fromUtf8(text.GetTextCStr(tmp)));
}

void plQtDocumentWindow::CreateImageCapture(const char* szOutputPath)
{
  PL_ASSERT_NOT_IMPLEMENTED;
}

bool plQtDocumentWindow::CanCloseWindow()
{
  return InternalCanCloseWindow();
}

bool plQtDocumentWindow::InternalCanCloseWindow()
{
  // I guess this is to remove the focus from other widgets like input boxes, such that they may modify the document.
  setFocus();
  clearFocus();

  if (m_pDocument && m_pDocument->IsModified())
  {
    QMessageBox::StandardButton res = plQtUiServices::MessageBoxQuestion("Save before closing?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      plStatus err = SaveDocument();

      if (err.Failed())
      {
        plQtUiServices::GetSingleton()->MessageBoxStatus(err, "Saving the scene failed.");
        return false;
      }
    }
  }

  return true;
}

void plQtDocumentWindow::CloseDocumentWindow()
{
  QMetaObject::invokeMethod(this, "SlotQueuedDelete", Qt::ConnectionType::QueuedConnection);
}

void plQtDocumentWindow::SlotQueuedDelete()
{
  setFocus();
  clearFocus();

  if (m_pDocument)
  {
    m_pDocument->GetDocumentManager()->CloseDocument(m_pDocument);
    return;
  }
  else
  {
    ShutdownDocumentWindow();
  }
}

void plQtDocumentWindow::OnPermanentGlobalStatusClicked(bool)
{
  plQtUiServices::Event e;
  e.m_Type = plQtUiServices::Event::ClickedDocumentPermanentStatusBarText;

  plQtUiServices::GetSingleton()->s_Events.Broadcast(e);
}

void plQtDocumentWindow::OnStatusBarMessageChanged(const QString& sNewText)
{
  QPalette pal = palette();

  if (sNewText.startsWith("Error:"))
  {
    pal.setColor(QPalette::WindowText, plToQtColor(plColorScheme::LightUI(plColorScheme::Red)));
  }
  else if (sNewText.startsWith("Warning:"))
  {
    pal.setColor(QPalette::WindowText, plToQtColor(plColorScheme::LightUI(plColorScheme::Yellow)));
  }
  else if (sNewText.startsWith("Note:"))
  {
    pal.setColor(QPalette::WindowText, plToQtColor(plColorScheme::LightUI(plColorScheme::Blue)));
  }

  statusBar()->setPalette(pal);
}

void plQtDocumentWindow::ShutdownDocumentWindow()
{
  SaveWindowLayout();

  InternalCloseDocumentWindow();

  plQtDocumentWindowEvent e;
  e.m_pWindow = this;
  e.m_Type = plQtDocumentWindowEvent::Type::WindowClosing;
  s_Events.Broadcast(e);

  InternalDeleteThis();

  e.m_Type = plQtDocumentWindowEvent::Type::WindowClosed;
  s_Events.Broadcast(e);
}

void plQtDocumentWindow::InternalCloseDocumentWindow() {}

void plQtDocumentWindow::EnsureVisible()
{
  m_pContainerWindow->EnsureVisible(this).IgnoreResult();
}

void plQtDocumentWindow::RequestWindowTabContextMenu(const QPoint& globalPos)
{
  plQtMenuActionMapView menu(nullptr);

  plActionContext context;
  context.m_sMapping = "DocumentWindowTabMenu";
  context.m_pDocument = GetDocument();
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(globalPos);
}

plQtDocumentWindow* plQtDocumentWindow::FindWindowByDocument(const plDocument* pDocument)
{
  // Sub-documents never have a window, so go to the main document instead
  pDocument = pDocument->GetMainDocument();

  for (auto pWnd : s_AllDocumentWindows)
  {
    if (pWnd->GetDocument() == pDocument)
      return pWnd;
  }

  return nullptr;
}

plQtContainerWindow* plQtDocumentWindow::GetContainerWindow() const
{
  return m_pContainerWindow;
}

plString plQtDocumentWindow::GetWindowIcon() const
{
  if (GetDocument() != nullptr)
    return GetDocument()->GetDocumentTypeDescriptor()->m_sIcon;

  return ":/GuiFoundation/PL-logo.svg";
}
