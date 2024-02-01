#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QSharedPointer>
#include <QToolBar>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class plActionMap;
class QAction;
class plQtProxy;
class QMenu;

class PL_GUIFOUNDATION_DLL plQtToolBarActionMapView : public QToolBar
{
  Q_OBJECT
  PL_DISALLOW_COPY_AND_ASSIGN(plQtToolBarActionMapView);

public:
  explicit plQtToolBarActionMapView(QString sTitle, QWidget* pParent);
  ~plQtToolBarActionMapView();

  void SetActionContext(const plActionContext& context);

  virtual void setVisible(bool bVisible) override;

private:
  void TreeEventHandler(const plDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();
  void CreateView(const plActionMap::TreeNode* pRoot);

private:
  plHashTable<plUuid, QSharedPointer<plQtProxy>> m_Proxies;

  plActionContext m_Context;
  plActionMap* m_pActionMap;
};

