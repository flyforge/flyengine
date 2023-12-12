#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonModel.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>

plQtSkeletonPanel::plQtSkeletonPanel(QWidget* pParent, plSkeletonAssetDocument* pDocument)
  : plQtDocumentPanel(pParent, pDocument)
{
  m_pSkeletonDocument = pDocument;

  setObjectName("SkeletonPanel");
  setWindowTitle("SKELETON");

  m_pMainWidget = new QWidget(this);
  m_pMainWidget->setLayout(new QVBoxLayout());
  m_pMainWidget->setContentsMargins(0, 0, 0, 0);
  m_pMainWidget->layout()->setContentsMargins(0, 0, 0, 0);
  m_pFilterWidget = new plQtSearchWidget(this);
  connect(m_pFilterWidget, &plQtSearchWidget::textChanged, this,
    [this](const QString& text) { m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text); });

  m_pMainWidget->layout()->addWidget(m_pFilterWidget);

  std::unique_ptr<plQtDocumentTreeModel> pModel(new plQtDocumentTreeModel(pDocument->GetObjectManager()));
  pModel->AddAdapter(new plQtDummyAdapter(pDocument->GetObjectManager(), plGetStaticRTTI<plDocumentRoot>(), "Children"));
  pModel->AddAdapter(new plQtDummyAdapter(pDocument->GetObjectManager(), plGetStaticRTTI<plEditableSkeleton>(), "Children"));
  pModel->AddAdapter(new plQtJointAdapter(pDocument));

  m_pTreeWidget = new plQtDocumentTreeView(this, pDocument, std::move(pModel));
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(false);
  m_pTreeWidget->expandAll();
  m_pMainWidget->layout()->addWidget(m_pTreeWidget);

  setWidget(m_pMainWidget);
}

plQtSkeletonPanel::~plQtSkeletonPanel() = default;
