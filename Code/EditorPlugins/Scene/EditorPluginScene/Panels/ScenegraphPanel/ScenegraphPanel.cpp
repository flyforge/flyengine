#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>

#include <QLayout>
#include <QStackedWidget>

namespace
{
  std::unique_ptr<plQtDocumentTreeModel> CreateGameObjectTreeModel(plSceneDocument* pDocument)
  {
    std::unique_ptr<plQtDocumentTreeModel> pModel(new plQtScenegraphModel(pDocument->GetObjectManager()));
    pModel->AddAdapter(new plQtDummyAdapter(pDocument->GetObjectManager(), plGetStaticRTTI<plDocumentRoot>(), "Children"));
    pModel->AddAdapter(new plQtGameObjectAdapter(pDocument->GetObjectManager()));
    return std::move(pModel);
  }

  std::unique_ptr<plQtDocumentTreeModel> CreateSceneTreeModel(plScene2Document* pDocument)
  {
    std::unique_ptr<plQtDocumentTreeModel> pModel(new plQtScenegraphModel(pDocument->GetSceneObjectManager()));
    pModel->AddAdapter(new plQtDummyAdapter(pDocument->GetSceneObjectManager(), plGetStaticRTTI<plDocumentRoot>(), "Children"));
    pModel->AddAdapter(new plQtGameObjectAdapter(pDocument->GetSceneObjectManager(), pDocument->GetSceneDocumentObjectMetaData(), pDocument->GetSceneGameObjectMetaData()));
    return std::move(pModel);
  }
} // namespace

plQtScenegraphPanel::plQtScenegraphPanel(QWidget* pParent, plSceneDocument* pDocument)
  : plQtDocumentPanel(pParent, pDocument)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("SCENEGRAPH");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel = CreateGameObjectTreeModel(pDocument);
  m_pMainGameObjectWidget = new plQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_pStack->addWidget(m_pMainGameObjectWidget);
}

plQtScenegraphPanel::plQtScenegraphPanel(QWidget* pParent, plScene2Document* pDocument)
  : plQtDocumentPanel(pParent, pDocument)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("SCENEGRAPH");
  m_pSceneDocument = pDocument;

  m_pStack = new QStackedWidget(this);
  m_pStack->setContentsMargins(0, 0, 0, 0);
  m_pStack->layout()->setContentsMargins(0, 0, 0, 0);
  setWidget(m_pStack);

  auto pCustomModel = CreateSceneTreeModel(pDocument);
  m_pMainGameObjectWidget = new plQtGameObjectWidget(this, pDocument, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel), pDocument->GetSceneSelectionManager());
  m_LayerWidgets[pDocument->GetGuid()] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);

  pDocument->m_LayerEvents.AddEventHandler(plMakeDelegate(&plQtScenegraphPanel::LayerEventHandler, this), m_LayerEventUnsubscriber);
  plHybridArray<plSceneDocument*, 16> layers;
  pDocument->GetLoadedLayers(layers);
  for (plSceneDocument* pLayer : layers)
  {
    if (pLayer != pDocument)
      LayerLoaded(pLayer->GetGuid());
  }
  ActiveLayerChanged(pDocument->GetActiveLayer());
}

plQtScenegraphPanel::~plQtScenegraphPanel()
{
}

void plQtScenegraphPanel::LayerEventHandler(const plScene2LayerEvent& e)
{
  switch (e.m_Type)
  {
    case plScene2LayerEvent::Type::LayerLoaded:
      LayerLoaded(e.m_layerGuid);
      break;
    case plScene2LayerEvent::Type::LayerUnloaded:
      LayerUnloaded(e.m_layerGuid);
      break;
    case plScene2LayerEvent::Type::ActiveLayerChanged:
    {
      ActiveLayerChanged(e.m_layerGuid);
    }
    default:
      break;
  }
}

void plQtScenegraphPanel::LayerLoaded(const plUuid& layerGuid)
{
  PLASMA_ASSERT_DEV(!m_LayerWidgets.Contains(layerGuid), "LayerLoaded was fired twice for the same layer.");

  auto pScene2 = static_cast<plScene2Document*>(m_pSceneDocument);
  auto pLayer = pScene2->GetLayerDocument(layerGuid);
  auto pCustomModel = CreateGameObjectTreeModel(pLayer);
  m_pMainGameObjectWidget = new plQtGameObjectWidget(this, pLayer, "EditorPluginScene_ScenegraphContextMenu", std::move(pCustomModel));
  m_LayerWidgets[layerGuid] = m_pMainGameObjectWidget;
  m_pStack->addWidget(m_pMainGameObjectWidget);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void plQtScenegraphPanel::LayerUnloaded(const plUuid& layerGuid)
{
  PLASMA_ASSERT_DEV(m_LayerWidgets.Contains(layerGuid), "LayerUnloaded was fired without the layer being loaded first.");

  plQtGameObjectWidget* pWidget = m_LayerWidgets[layerGuid];
  m_pStack->removeWidget(pWidget);
  m_LayerWidgets.Remove(layerGuid);
  delete pWidget;

  auto pScene2 = static_cast<plScene2Document*>(m_pSceneDocument);
  ActiveLayerChanged(pScene2->GetActiveLayer());
}

void plQtScenegraphPanel::ActiveLayerChanged(const plUuid& layerGuid)
{
  m_pStack->setCurrentWidget(m_LayerWidgets[layerGuid]);
}
