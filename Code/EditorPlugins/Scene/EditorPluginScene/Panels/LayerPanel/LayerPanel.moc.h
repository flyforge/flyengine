#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class plScene2Document;
class plQtLayerDelegate;

class plQtLayerPanel : public plQtDocumentPanel
{
  Q_OBJECT

public:
  plQtLayerPanel(QWidget* pParent, plScene2Document* pDocument);
  ~plQtLayerPanel();

private Q_SLOTS:
  void OnRequestContextMenu(QPoint pos);

private:
  plQtLayerDelegate* m_pDelegate = nullptr;
  plScene2Document* m_pSceneDocument = nullptr;
  plQtDocumentTreeView* m_pTreeWidget = nullptr;
  plString m_sContextMenuMapping;
};
