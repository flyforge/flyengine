#pragma once

#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <Foundation/Basics.h>

class plQtSearchWidget;
class plQtDocumentTreeView;
class plSceneDocument;
class plScene2Document;
class QStackedWidget;
struct plScene2LayerEvent;

class plQtScenegraphPanel : public plQtDocumentPanel
{
  Q_OBJECT

public:
  plQtScenegraphPanel(QWidget* pParent, plSceneDocument* pDocument);
  plQtScenegraphPanel(QWidget* pParent, plScene2Document* pDocument);
  ~plQtScenegraphPanel();

private:
  void LayerEventHandler(const plScene2LayerEvent& e);
  void LayerLoaded(const plUuid& layerGuid);
  void LayerUnloaded(const plUuid& layerGuid);
  void ActiveLayerChanged(const plUuid& layerGuid);

private:
  plSceneDocument* m_pSceneDocument;
  QStackedWidget* m_pStack = nullptr;
  plQtGameObjectWidget* m_pMainGameObjectWidget = nullptr;
  plEvent<const plScene2LayerEvent&>::Unsubscriber m_LayerEventUnsubscriber;
  plMap<plUuid, plQtGameObjectWidget*> m_LayerWidgets;
};

