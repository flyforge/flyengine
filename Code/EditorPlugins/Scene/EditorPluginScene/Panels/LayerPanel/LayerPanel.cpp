#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerAdapter.moc.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphModel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QVBoxLayout>

plQtLayerPanel::plQtLayerPanel(QWidget* pParent, plScene2Document* pDocument)
  : plQtDocumentPanel(pParent, pDocument)
{
  setObjectName("LayerPanel");
  setWindowTitle("Layers");
  m_pSceneDocument = pDocument;
  m_pDelegate = new plQtLayerDelegate(this, pDocument);

  std::unique_ptr<plQtLayerModel> pModel(new plQtLayerModel(m_pSceneDocument));
  pModel->AddAdapter(new plQtDummyAdapter(pDocument->GetSceneObjectManager(), plGetStaticRTTI<plSceneDocumentSettings>(), "Layers"));
  pModel->AddAdapter(new plQtLayerAdapter(pDocument));

  m_pTreeWidget = new plQtDocumentTreeView(this, pDocument, std::move(pModel), m_pSceneDocument->GetLayerSelectionManager());
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(false);
  m_pTreeWidget->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  m_pTreeWidget->setItemDelegate(m_pDelegate);

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  PL_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr,
    "signal/slot connection failed");

  setWidget(m_pTreeWidget);
}

plQtLayerPanel::~plQtLayerPanel() = default;

void plQtLayerPanel::OnRequestContextMenu(QPoint pos)
{
  plQtMenuActionMapView menu(nullptr);

  plActionContext context;
  context.m_sMapping = "EditorPluginScene_LayerContextMenu";
  context.m_pDocument = m_pSceneDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}
