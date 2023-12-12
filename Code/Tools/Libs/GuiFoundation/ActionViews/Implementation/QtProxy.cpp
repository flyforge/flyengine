#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QAction>
#include <QBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QSlider>
#include <QWidgetAction>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

plRttiMappedObjectFactory<plQtProxy> plQtProxy::s_Factory;
plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>> plQtProxy::s_GlobalActions;
plMap<const plDocument*, plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>>> plQtProxy::s_DocumentActions;
plMap<QWidget*, plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>>> plQtProxy::s_WindowActions;
QObject* plQtProxy::s_pSignalProxy = nullptr;

static plQtProxy* QtMenuProxyCreator(const plRTTI* pRtti)
{
  return new (plQtMenuProxy);
}

static plQtProxy* QtCategoryProxyCreator(const plRTTI* pRtti)
{
  return new (plQtCategoryProxy);
}

static plQtProxy* QtButtonProxyCreator(const plRTTI* pRtti)
{
  return new (plQtButtonProxy);
}

static plQtProxy* QtDynamicMenuProxyCreator(const plRTTI* pRtti)
{
  return new (plQtDynamicMenuProxy);
}

static plQtProxy* QtDynamicActionAndMenuProxyCreator(const plRTTI* pRtti)
{
  return new (plQtDynamicActionAndMenuProxy);
}

static plQtProxy* QtSliderProxyCreator(const plRTTI* pRtti)
{
  return new (plQtSliderProxy);
}

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtProxies)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "ToolsFoundation",
  "ActionManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plQtProxy::GetFactory().RegisterCreator(plGetStaticRTTI<plMenuAction>(), QtMenuProxyCreator);
    plQtProxy::GetFactory().RegisterCreator(plGetStaticRTTI<plCategoryAction>(), QtCategoryProxyCreator);
    plQtProxy::GetFactory().RegisterCreator(plGetStaticRTTI<plDynamicMenuAction>(), QtDynamicMenuProxyCreator);
    plQtProxy::GetFactory().RegisterCreator(plGetStaticRTTI<plDynamicActionAndMenuAction>(), QtDynamicActionAndMenuProxyCreator);
    plQtProxy::GetFactory().RegisterCreator(plGetStaticRTTI<plButtonAction>(), QtButtonProxyCreator);
    plQtProxy::GetFactory().RegisterCreator(plGetStaticRTTI<plSliderAction>(), QtSliderProxyCreator);
    plQtProxy::s_pSignalProxy = new QObject;
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plQtProxy::GetFactory().UnregisterCreator(plGetStaticRTTI<plMenuAction>());
    plQtProxy::GetFactory().UnregisterCreator(plGetStaticRTTI<plCategoryAction>());
    plQtProxy::GetFactory().UnregisterCreator(plGetStaticRTTI<plDynamicMenuAction>());
    plQtProxy::GetFactory().UnregisterCreator(plGetStaticRTTI<plDynamicActionAndMenuAction>());
    plQtProxy::GetFactory().UnregisterCreator(plGetStaticRTTI<plButtonAction>());
    plQtProxy::GetFactory().UnregisterCreator(plGetStaticRTTI<plSliderAction>());
    plQtProxy::s_GlobalActions.Clear();
    plQtProxy::s_DocumentActions.Clear();
    plQtProxy::s_WindowActions.Clear();
    delete plQtProxy::s_pSignalProxy;
    plQtProxy::s_pSignalProxy = nullptr;
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool plQtProxy::TriggerDocumentAction(plDocument* pDocument, QKeyEvent* event)
{
  auto CheckActions = [](QKeyEvent* event, plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>>& actions) -> bool {
    for (auto weakActionProxy : actions)
    {
      if (auto pProxy = weakActionProxy.Value().toStrongRef())
      {
        QAction* pQAction = nullptr;
        if (auto pActionProxy = qobject_cast<plQtActionProxy*>(pProxy))
        {
          pQAction = pActionProxy->GetQAction();
        }
        else if (auto pActionProxy2 = qobject_cast<plQtDynamicActionAndMenuProxy*>(pProxy))
        {
          pQAction = pActionProxy2->GetQAction();
        }

        if (pQAction)
        {
          QKeySequence ks = pQAction->shortcut();
          if (pQAction->isEnabled() && QKeySequence(event->key() | event->modifiers()) == ks)
          {
            pQAction->trigger();
            event->accept();
            return true;
          }
        }
      }
    }
    return false;
  };

  if (pDocument)
  {
    plMap<plActionDescriptorHandle, QWeakPointer<plQtProxy>>& actions = s_DocumentActions[pDocument];
    if (CheckActions(event, actions))
      return true;
  }
  return CheckActions(event, s_GlobalActions);
}

plRttiMappedObjectFactory<plQtProxy>& plQtProxy::GetFactory()
{
  return s_Factory;
}
QSharedPointer<plQtProxy> plQtProxy::GetProxy(plActionContext& context, plActionDescriptorHandle hDesc)
{
  QSharedPointer<plQtProxy> pProxy;
  const plActionDescriptor* pDesc = hDesc.GetDescriptor();
  if (pDesc->m_Type != plActionType::Action && pDesc->m_Type != plActionType::ActionAndMenu)
  {
    auto pAction = pDesc->CreateAction(context);
    pProxy = QSharedPointer<plQtProxy>(plQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
    PLASMA_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
    pProxy->SetAction(pAction);
    PLASMA_ASSERT_DEV(pProxy->GetAction()->GetContext().m_pDocument == context.m_pDocument, "invalid document pointer");
    return pProxy;
  }

  // plActionType::Action will be cached to ensure only one QAction exist in its scope to prevent shortcut collisions.
  switch (pDesc->m_Scope)
  {
    case plActionScope::Global:
    {
      QWeakPointer<plQtProxy> pTemp = s_GlobalActions[hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(context);
        pProxy = QSharedPointer<plQtProxy>(plQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        PLASMA_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        s_GlobalActions[hDesc] = pProxy.toWeakRef();
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }

    case plActionScope::Document:
    {
      const plDocument* pDocument = context.m_pDocument; // may be null

      QWeakPointer<plQtProxy> pTemp = s_DocumentActions[pDocument][hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(context);
        pProxy = QSharedPointer<plQtProxy>(plQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        PLASMA_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        s_DocumentActions[pDocument][hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }

    case plActionScope::Window:
    {
      bool bExisted = true;
      auto it = s_WindowActions.FindOrAdd(context.m_pWindow, &bExisted);
      if (!bExisted)
      {
        s_pSignalProxy->connect(context.m_pWindow, &QObject::destroyed, s_pSignalProxy, [=]() { s_WindowActions.Remove(context.m_pWindow); });
      }
      QWeakPointer<plQtProxy> pTemp = it.Value()[hDesc];
      if (pTemp.isNull())
      {
        auto pAction = pDesc->CreateAction(context);
        pProxy = QSharedPointer<plQtProxy>(plQtProxy::GetFactory().CreateObject(pAction->GetDynamicRTTI()));
        PLASMA_ASSERT_DEBUG(pProxy != nullptr, "No proxy assigned to action '{0}'", pDesc->m_sActionName);
        pProxy->SetAction(pAction);
        it.Value()[hDesc] = pProxy;
      }
      else
      {
        pProxy = pTemp.toStrongRef();
      }

      break;
    }
  }

  // make sure we don't use actions that are meant for a different document
  if (pProxy != nullptr && pProxy->GetAction()->GetContext().m_pDocument != nullptr)
  {
    // if this assert fires, you might have tried to map an action into multiple documents, which uses plActionScope::Global
    plAction* pAction = pProxy->GetAction();
    const plActionContext& ctxt = pAction->GetContext();
    plDocument* pDoc = ctxt.m_pDocument;
    PLASMA_ASSERT_DEV(pDoc == context.m_pDocument, "invalid document pointer");
  }
  return pProxy;
}

plQtProxy::plQtProxy()
{
  m_pAction = nullptr;
}

plQtProxy::~plQtProxy()
{
  if (m_pAction != nullptr)
    plActionManager::GetActionDescriptor(m_pAction->GetDescriptorHandle())->DeleteAction(m_pAction);
}

void plQtProxy::SetAction(plAction* pAction)
{
  m_pAction = pAction;
}

//////////////////// plQtMenuProxy /////////////////////

plQtMenuProxy::plQtMenuProxy()
{
  m_pMenu = nullptr;
}

plQtMenuProxy::~plQtMenuProxy()
{
  m_pMenu->deleteLater();
  delete m_pMenu;
}

void plQtMenuProxy::Update()
{
  auto pMenu = static_cast<plMenuAction*>(m_pAction);

  m_pMenu->setIcon(plQtUiServices::GetCachedIconResource(pMenu->GetIconPath()));
  m_pMenu->setTitle(QString::fromUtf8(plTranslate(pMenu->GetName())));
}

void plQtMenuProxy::SetAction(plAction* pAction)
{
  plQtProxy::SetAction(pAction);

  m_pMenu = new QMenu();
  m_pMenu->setToolTipsVisible(true);
  Update();
}

QMenu* plQtMenuProxy::GetQMenu()
{
  return m_pMenu;
}

//////////////////////////////////////////////////////////////////////////
//////////////////// plQtButtonProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

plQtButtonProxy::plQtButtonProxy()
{
  m_pQtAction = nullptr;
}

plQtButtonProxy::~plQtButtonProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(plMakeDelegate(&plQtButtonProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}

void plQtButtonProxy::Update()
{
  if (m_pQtAction == nullptr)
    return;

  auto pButton = static_cast<plButtonAction*>(m_pAction);


  const plActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();
  m_pQtAction->setShortcut(QKeySequence(QString::fromUtf8(pDesc->m_sShortcut.GetData())));

  const QString sDisplayShortcut = m_pQtAction->shortcut().toString(QKeySequence::NativeText);
  QString sTooltip = plTranslateTooltip(pButton->GetName());

  plStringBuilder sDisplay = plTranslate(pButton->GetName());

  if (sTooltip.isEmpty())
  {
    sTooltip = sDisplay;
    sTooltip.replace("&", "");
  }

  if (!sDisplayShortcut.isEmpty())
  {
    sTooltip.append(" (");
    sTooltip.append(sDisplayShortcut);
    sTooltip.append(")");
  }

  if (!plStringUtils::IsNullOrEmpty(pButton->GetAdditionalDisplayString()))
    sDisplay.Append(" '", pButton->GetAdditionalDisplayString(), "'"); // TODO: translate this as well?

  m_pQtAction->setIcon(plQtUiServices::GetCachedIconResource(pButton->GetIconPath()));
  m_pQtAction->setText(QString::fromUtf8(sDisplay.GetData()));
  m_pQtAction->setToolTip(sTooltip);
  m_pQtAction->setCheckable(pButton->IsCheckable());
  m_pQtAction->setChecked(pButton->IsChecked());
  m_pQtAction->setEnabled(pButton->IsEnabled());
  m_pQtAction->setVisible(pButton->IsVisible());
}


void SetupQAction(plAction* pAction, QPointer<QAction>& pQtAction, QObject* pTarget)
{
  plActionDescriptorHandle hDesc = pAction->GetDescriptorHandle();
  const plActionDescriptor* pDesc = hDesc.GetDescriptor();

  if (pQtAction == nullptr)
  {
    pQtAction = new QAction(nullptr);
    PLASMA_VERIFY(QObject::connect(pQtAction, SIGNAL(triggered(bool)), pTarget, SLOT(OnTriggered())) != nullptr, "connection failed");

    switch (pDesc->m_Scope)
    {
      case plActionScope::Global:
      {
        // Parent is null so the global actions don't get deleted.
        pQtAction->setShortcutContext(Qt::ShortcutContext::ApplicationShortcut);
      }
      break;
      case plActionScope::Document:
      {
        // Parent is set to the window belonging to the document.
        plQtDocumentWindow* pWindow = plQtDocumentWindow::FindWindowByDocument(pAction->GetContext().m_pDocument);
        PLASMA_ASSERT_DEBUG(pWindow != nullptr, "You can't map a plActionScope::Document action without that document existing!");
        pQtAction->setParent(pWindow);
        pQtAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
      }
      break;
      case plActionScope::Window:
      {
        pQtAction->setParent(pAction->GetContext().m_pWindow);
        pQtAction->setShortcutContext(Qt::ShortcutContext::WidgetWithChildrenShortcut);
      }
      break;
    }
  }
}

void plQtButtonProxy::SetAction(plAction* pAction)
{
  PLASMA_ASSERT_DEV(m_pAction == nullptr, "Es darf nicht sein, es kann nicht sein!");

  plQtProxy::SetAction(pAction);
  m_pAction->m_StatusUpdateEvent.AddEventHandler(plMakeDelegate(&plQtButtonProxy::StatusUpdateEventHandler, this));

  SetupQAction(m_pAction, m_pQtAction, this);

  Update();
}

QAction* plQtButtonProxy::GetQAction()
{
  return m_pQtAction;
}

void plQtButtonProxy::StatusUpdateEventHandler(plAction* pAction)
{
  Update();
}

void plQtButtonProxy::OnTriggered()
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  m_pAction->Execute(m_pQtAction->isChecked());

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void plQtDynamicMenuProxy::SetAction(plAction* pAction)
{
  plQtMenuProxy::SetAction(pAction);

  PLASMA_VERIFY(connect(m_pMenu, SIGNAL(aboutToShow()), this, SLOT(SlotMenuAboutToShow())) != nullptr, "signal/slot connection failed");
}

void plQtDynamicMenuProxy::SlotMenuAboutToShow()
{
  m_pMenu->clear();

  static_cast<plDynamicMenuAction*>(m_pAction)->GetEntries(m_Entries);

  if (m_Entries.IsEmpty())
  {
    m_pMenu->addAction("<empty>")->setEnabled(false);
  }
  else
  {
    for (plUInt32 i = 0; i < m_Entries.GetCount(); ++i)
    {
      const auto& p = m_Entries[i];

      if (p.m_ItemFlags.IsSet(plDynamicMenuAction::Item::ItemFlags::Separator))
      {
        m_pMenu->addSeparator();
      }
      else
      {
        auto pAction = m_pMenu->addAction(QString::fromUtf8(p.m_sDisplay.GetData()));
        pAction->setData(i);
        pAction->setIcon(p.m_Icon);
        pAction->setCheckable(p.m_CheckState != plDynamicMenuAction::Item::CheckMark::NotCheckable);
        pAction->setChecked(p.m_CheckState == plDynamicMenuAction::Item::CheckMark::Checked);

        PLASMA_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(SlotMenuEntryTriggered())) != nullptr, "signal/slot connection failed");
      }
    }
  }
}

void plQtDynamicMenuProxy::SlotMenuEntryTriggered()
{
  QAction* pAction = qobject_cast<QAction*>(sender());
  if (!pAction)
    return;

  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  plUInt32 index = pAction->data().toUInt();
  m_pAction->Execute(m_Entries[index].m_UserValue);

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

//////////////////////////////////////////////////////////////////////////
//////////////////// plQtDynamicActionAndMenuProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

plQtDynamicActionAndMenuProxy::plQtDynamicActionAndMenuProxy()
{
  m_pQtAction = nullptr;
}

plQtDynamicActionAndMenuProxy::~plQtDynamicActionAndMenuProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(plMakeDelegate(&plQtDynamicActionAndMenuProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}


void plQtDynamicActionAndMenuProxy::Update()
{
  plQtDynamicMenuProxy::Update();

  if (m_pQtAction == nullptr)
    return;

  auto pButton = static_cast<plDynamicActionAndMenuAction*>(m_pAction);

  const plActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();
  m_pQtAction->setShortcut(QKeySequence(QString::fromUtf8(pDesc->m_sShortcut.GetData())));

  plStringBuilder sDisplay = plTranslate(pButton->GetName());

  if (!plStringUtils::IsNullOrEmpty(pButton->GetAdditionalDisplayString()))
    sDisplay.Append(" '", pButton->GetAdditionalDisplayString(), "'"); // TODO: translate this as well?

  const QString sDisplayShortcut = m_pQtAction->shortcut().toString(QKeySequence::NativeText);
  QString sTooltip = plTranslateTooltip(pButton->GetName());

  if (sTooltip.isEmpty())
  {
    sTooltip = sDisplay;
    sTooltip.replace("&", "");
  }

  if (!sDisplayShortcut.isEmpty())
  {
    sTooltip.append(" (");
    sTooltip.append(sDisplayShortcut);
    sTooltip.append(")");
  }

  m_pQtAction->setIcon(plQtUiServices::GetCachedIconResource(pButton->GetIconPath()));
  m_pQtAction->setText(QString::fromUtf8(sDisplay.GetData()));
  m_pQtAction->setToolTip(sTooltip);
  m_pQtAction->setEnabled(pButton->IsEnabled());
  m_pQtAction->setVisible(pButton->IsVisible());
}


void plQtDynamicActionAndMenuProxy::SetAction(plAction* pAction)
{
  plQtDynamicMenuProxy::SetAction(pAction);

  m_pAction->m_StatusUpdateEvent.AddEventHandler(plMakeDelegate(&plQtDynamicActionAndMenuProxy::StatusUpdateEventHandler, this));

  SetupQAction(m_pAction, m_pQtAction, this);

  Update();
}

QAction* plQtDynamicActionAndMenuProxy::GetQAction()
{
  return m_pQtAction;
}

void plQtDynamicActionAndMenuProxy::OnTriggered()
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  m_pAction->Execute(plVariant());

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void plQtDynamicActionAndMenuProxy::StatusUpdateEventHandler(plAction* pAction)
{
  Update();
}


//////////////////////////////////////////////////////////////////////////
//////////////////// plQtSliderProxy /////////////////////
//////////////////////////////////////////////////////////////////////////

plQtSliderWidgetAction::plQtSliderWidgetAction(QWidget* parent)
  : QWidgetAction(parent)
{
}

plQtLabeledSlider::plQtLabeledSlider(QWidget* parent)
  : QWidget(parent)
{
  m_pLabel = new QLabel(this);
  m_pSlider = new QSlider(this);
  setLayout(new QHBoxLayout(this));

  layout()->addWidget(m_pLabel);
  layout()->addWidget(m_pSlider);

  setMaximumWidth(300);
}

void plQtSliderWidgetAction::setMinimum(int value)
{
  m_iMinimum = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    plQtLabeledSlider* pGroup = qobject_cast<plQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setMinimum(m_iMinimum);
  }
}

void plQtSliderWidgetAction::setMaximum(int value)
{
  m_iMaximum = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    plQtLabeledSlider* pGroup = qobject_cast<plQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setMaximum(m_iMaximum);
  }
}

void plQtSliderWidgetAction::setValue(int value)
{
  m_iValue = value;

  const QList<QWidget*> widgets = createdWidgets();

  for (QWidget* pWidget : widgets)
  {
    plQtLabeledSlider* pGroup = qobject_cast<plQtLabeledSlider*>(pWidget);
    pGroup->m_pSlider->setValue(m_iValue);
  }
}

void plQtSliderWidgetAction::OnValueChanged(int value)
{
  Q_EMIT valueChanged(value);
}

QWidget* plQtSliderWidgetAction::createWidget(QWidget* parent)
{
  plQtLabeledSlider* pGroup = new plQtLabeledSlider(parent);
  pGroup->m_pSlider->setOrientation(Qt::Orientation::Horizontal);

  PLASMA_VERIFY(connect(pGroup->m_pSlider, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged(int))) != nullptr, "connection failed");

  pGroup->m_pLabel->setText(text());
  pGroup->m_pLabel->installEventFilter(this);
  pGroup->m_pLabel->setToolTip(toolTip());
  pGroup->installEventFilter(this);
  pGroup->m_pSlider->setMinimum(m_iMinimum);
  pGroup->m_pSlider->setMaximum(m_iMaximum);
  pGroup->m_pSlider->setValue(m_iValue);
  pGroup->m_pSlider->setToolTip(toolTip());

  return pGroup;
}

bool plQtSliderWidgetAction::eventFilter(QObject* obj, QEvent* e)
{
  if (e->type() == QEvent::Type::MouseButtonPress || e->type() == QEvent::Type::MouseButtonRelease || e->type() == QEvent::Type::MouseButtonDblClick)
  {
    e->accept();
    return true;
  }

  return false;
}

plQtSliderProxy::plQtSliderProxy()
{
  m_pQtAction = nullptr;
}

plQtSliderProxy::~plQtSliderProxy()
{
  m_pAction->m_StatusUpdateEvent.RemoveEventHandler(plMakeDelegate(&plQtSliderProxy::StatusUpdateEventHandler, this));

  if (m_pQtAction != nullptr)
  {
    m_pQtAction->deleteLater();
  }
  m_pQtAction = nullptr;
}

void plQtSliderProxy::Update()
{
  if (m_pQtAction == nullptr)
    return;

  auto pAction = static_cast<plSliderAction*>(m_pAction);

  const plActionDescriptor* pDesc = m_pAction->GetDescriptorHandle().GetDescriptor();

  plQtSliderWidgetAction* pSliderAction = qobject_cast<plQtSliderWidgetAction*>(m_pQtAction);
  plQtScopedBlockSignals bs(pSliderAction);

  plInt32 minVal, maxVal;
  pAction->GetRange(minVal, maxVal);
  pSliderAction->setMinimum(minVal);
  pSliderAction->setMaximum(maxVal);
  pSliderAction->setValue(pAction->GetValue());
  pSliderAction->setText(plTranslate(pAction->GetName()));
  pSliderAction->setToolTip(plTranslateTooltip(pAction->GetName()));
  pSliderAction->setEnabled(pAction->IsEnabled());
  pSliderAction->setVisible(pAction->IsVisible());
}

void plQtSliderProxy::SetAction(plAction* pAction)
{
  PLASMA_ASSERT_DEV(m_pAction == nullptr, "Es darf nicht sein, es kann nicht sein!");

  plQtProxy::SetAction(pAction);
  m_pAction->m_StatusUpdateEvent.AddEventHandler(plMakeDelegate(&plQtSliderProxy::StatusUpdateEventHandler, this));

  plActionDescriptorHandle hDesc = m_pAction->GetDescriptorHandle();
  const plActionDescriptor* pDesc = hDesc.GetDescriptor();

  if (m_pQtAction == nullptr)
  {
    m_pQtAction = new plQtSliderWidgetAction(nullptr);

    PLASMA_VERIFY(connect(m_pQtAction, SIGNAL(valueChanged(int)), this, SLOT(OnValueChanged(int))) != nullptr, "connection failed");
  }

  Update();
}

QAction* plQtSliderProxy::GetQAction()
{
  return m_pQtAction;
}


void plQtSliderProxy::OnValueChanged(int value)
{
  // make sure all focus is lost, to trigger pending changes
  QPointer<QWidget> pFocusWidget = QApplication::focusWidget();
  if (pFocusWidget)
    QApplication::focusWidget()->clearFocus();

  // make sure all instances of the slider get updated, by setting the new value
  m_pQtAction->setValue(value);
  m_pAction->Execute(value);

  if (pFocusWidget)
    pFocusWidget->setFocus();
}

void plQtSliderProxy::StatusUpdateEventHandler(plAction* pAction)
{
  Update();
}
