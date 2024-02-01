#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

plQtMenuActionMapView::plQtMenuActionMapView(QWidget* pParent)
{
  setToolTipsVisible(true);
}

plQtMenuActionMapView::~plQtMenuActionMapView()
{
  ClearView();
}

void plQtMenuActionMapView::SetActionContext(const plActionContext& context)
{
  auto pMap = plActionMapManager::GetActionMap(context.m_sMapping);

  PL_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void plQtMenuActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void plQtMenuActionMapView::AddDocumentObjectToMenu(plHashTable<plUuid, QSharedPointer<plQtProxy>>& ref_proxies, plActionContext& ref_context,
  plActionMap* pActionMap, QMenu* pCurrentRoot, const plActionMap::TreeNode* pObject)
{
  if (pObject == nullptr)
    return;

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = pActionMap->GetDescriptor(pChild);
    QSharedPointer<plQtProxy> pProxy = plQtProxy::GetProxy(ref_context, pDesc->m_hAction);
    ref_proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case plActionType::Action:
      {
        QAction* pQtAction = static_cast<plQtActionProxy*>(pProxy.data())->GetQAction();
        pCurrentRoot->addAction(pQtAction);
      }
      break;

      case plActionType::Category:
      {
        pCurrentRoot->addSeparator();

        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pCurrentRoot, pChild);

        pCurrentRoot->addSeparator();
      }
      break;

      case plActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<plQtMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;

      case plActionType::ActionAndMenu:
      {
        QAction* pQtAction = static_cast<plQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQAction();
        QMenu* pQtMenu = static_cast<plQtDynamicActionAndMenuProxy*>(pProxy.data())->GetQMenu();
        pCurrentRoot->addAction(pQtAction);
        pCurrentRoot->addMenu(pQtMenu);
        AddDocumentObjectToMenu(ref_proxies, ref_context, pActionMap, pQtMenu, pChild);
      }
      break;
    }
  }
}

void plQtMenuActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, this, pObject);
}
