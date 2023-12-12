#pragma once

#include <GuiFoundation/Action/ActionMap.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QMenu>
#include <QSharedPointer>
#include <ToolsFoundation/Factory/RttiMappedObjectFactory.h>

class QWidget;
class plActionMap;
class QAction;
class plQtProxy;


class PLASMA_GUIFOUNDATION_DLL plQtMenuActionMapView : public QMenu
{
  Q_OBJECT
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plQtMenuActionMapView);

public:
  explicit plQtMenuActionMapView(QWidget* parent);
  ~plQtMenuActionMapView();

  void SetActionContext(const plActionContext& context);

  static void AddDocumentObjectToMenu(plHashTable<plUuid, QSharedPointer<plQtProxy>>& Proxies, plActionContext& Context, plActionMap* pActionMap,
    QMenu* pCurrentRoot, const plActionMap::TreeNode* pObject);

private:
  void ClearView();
  void CreateView();

private:
  plHashTable<plUuid, QSharedPointer<plQtProxy>> m_Proxies;

  plActionContext m_Context;
  plActionMap* m_pActionMap;
};

