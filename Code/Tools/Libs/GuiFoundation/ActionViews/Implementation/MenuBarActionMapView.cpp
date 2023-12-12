#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>

plQtMenuBarActionMapView::plQtMenuBarActionMapView(QWidget* parent)
  : QMenuBar(parent)
{
}

plQtMenuBarActionMapView::~plQtMenuBarActionMapView()
{
  ClearView();
}

void plQtMenuBarActionMapView::SetActionContext(const plActionContext& context)
{
  auto pMap = plActionMapManager::GetActionMap(context.m_sMapping);

  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping '{0}' does not exist", context.m_sMapping);

  m_pActionMap = pMap;
  m_Context = context;

  CreateView();
}

void plQtMenuBarActionMapView::ClearView()
{
  m_Proxies.Clear();
}

void plQtMenuBarActionMapView::CreateView()
{
  ClearView();

  auto pObject = m_pActionMap->GetRootObject();

  for (auto pChild : pObject->GetChildren())
  {
    auto pDesc = m_pActionMap->GetDescriptor(pChild);

    QSharedPointer<plQtProxy> pProxy = plQtProxy::GetProxy(m_Context, pDesc->m_hAction);
    m_Proxies[pChild->GetGuid()] = pProxy;

    switch (pDesc->m_hAction.GetDescriptor()->m_Type)
    {
      case plActionType::Action:
      {
        PLASMA_REPORT_FAILURE("Cannot map actions in a menubar view!");
      }
      break;

      case plActionType::Category:
      {
        PLASMA_REPORT_FAILURE("Cannot map category in a menubar view!");
      }
      break;

      case plActionType::Menu:
      {
        QMenu* pQtMenu = static_cast<plQtMenuProxy*>(pProxy.data())->GetQMenu();
        addMenu(pQtMenu);
        plQtMenuActionMapView::AddDocumentObjectToMenu(m_Proxies, m_Context, m_pActionMap, pQtMenu, pChild);
      }
      break;

      case plActionType::ActionAndMenu:
      {
        PLASMA_REPORT_FAILURE("Cannot map ActionAndMenu in a menubar view!");
      }
      break;
    }
  }
}
