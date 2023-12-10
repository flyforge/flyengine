#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QMenu>
#include <QToolButton>

plQtToolBarActionMapView::plQtToolBarActionMapView(QString sTitle, QWidget* pParent)
  : QToolBar(sTitle, pParent)
{
  setIconSize(QSize(16, 16));
  setFloatable(false);

  toggleViewAction()->setEnabled(false);
}

plQtToolBarActionMapView::~plQtToolBarActionMapView()
{
  ClearView();
}

void plQtToolBarActionMapView::SetActionContext(const plActionContext& context)
{
  auto pMap = plActionMapManager::GetActionMap(context.m_sMapping);

  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void plQtToolBarActionMapView::setVisible(bool bVisible)
{
  QToolBar::setVisible(true);
}

void plQtToolBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void plQtToolBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  CreateView(pObject);

  if (!actions().isEmpty() && actions().back()->isSeparator())
  {
    QAction* pAction = actions().back();
    removeAction(pAction);
    pAction->deleteLater();
  }
}

void plQtToolBarActionMapView::CreateView(const plActionMap::TreeNode* pObject)
{
  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);
    QSharedPointer<plQtProxy> pProxy = plQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case plActionType::Action:
      {
        QAction* pQtAction = static_cast<plQtActionProxy*>(pProxy.data())->GetQAction();
        addAction(pQtAction);
      }
      break;

      case plActionType::Category:
      {
        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());

        CreateView(pChild);

        if (!actions().isEmpty() && !actions().back()->isSeparator())
          addSeparator()->setParent(pProxy.data());
      }
      break;

      case plActionType::Menu:
      {
        plNamedAction* pNamed = static_cast<plNamedAction*>(pProxy->GetAction());

        QMenu* pQtMenu = static_cast<plQtMenuProxy*>(pProxy.data())->GetQMenu();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
        pButton->setText(pQtMenu->title());
        pButton->setIcon(plQtUiServices::GetCachedIconResource(pNamed->GetIconPath()));
        pButton->setToolTip(pQtMenu->title().toUtf8().data());

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        plQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

      case plActionType::ActionAndMenu:
      {
        QMenu* pQtMenu = static_cast<plQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        QAction* pQtAction = static_cast<plQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        // TODO pButton leaks!
        QToolButton* pButton = new QToolButton(this);
        pButton->setDefaultAction(pQtAction);
        pButton->setMenu(pQtMenu);
        pButton->setPopupMode(QToolButton::ToolButtonPopupMode::MenuButtonPopup);

        // TODO addWidget return value of QAction leaks!
        QAction* pToolButtonAction = addWidget(pButton);
        pToolButtonAction->setParent(pQtMenu);

        plQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}
