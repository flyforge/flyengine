#pragma once

#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <Foundation/Basics.h>

class plQtSceneViewWidget : public plQtGameObjectViewWidget
{
  Q_OBJECT
public:
  plQtSceneViewWidget(QWidget* pParent, plQtGameObjectDocumentWindow* pOwnerWindow, PlasmaEngineViewConfig* pViewConfig);
  ~plQtSceneViewWidget();

  virtual bool IsPickingAgainstSelectionAllowed() const override;

protected:
  virtual void dragEnterEvent(QDragEnterEvent* e) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* e) override;
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* e) override;
  virtual void OnOpenContextMenu(QPoint globalPos) override;

  bool m_bAllowPickSelectedWhileDragging;
  plTime m_LastDragMoveEvent;

  static bool s_bContextMenuInitialized;
};

