#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QMenuBar>
#include <QSharedPointer>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class plActionMap;
class QAction;
class plQtProxy;

class PL_GUIFOUNDATION_DLL plQtMenuBarActionMapView : public QMenuBar
{
  Q_OBJECT
  PL_DISALLOW_COPY_AND_ASSIGN(plQtMenuBarActionMapView);

public:
  explicit plQtMenuBarActionMapView(QWidget* pParent);
  ~plQtMenuBarActionMapView();

  void SetActionContext(const plActionContext& context);

private:
  void TreeEventHandler(const plDocumentObjectStructureEvent& e);
  void TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e);

  void ClearView();
  void CreateView();

private:
  plHashTable<plUuid, QSharedPointer<plQtProxy>> m_Proxies;

  plActionContext m_Context;
  plActionMap* m_pActionMap;
};

