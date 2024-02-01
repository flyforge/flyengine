#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>


plQtGameObjectWidget::plQtGameObjectWidget(QWidget* pParent, plGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<plQtDocumentTreeModel> pCustomModel, plSelectionManager* pSelection)
{
  m_pDocument = pDocument;
  m_sContextMenuMapping = szContextMenuMapping;

  setLayout(new QVBoxLayout());
  setContentsMargins(0, 0, 0, 0);
  layout()->setContentsMargins(0, 0, 0, 0);

  m_pFilterWidget = new plQtSearchWidget(this);
  connect(m_pFilterWidget, &plQtSearchWidget::textChanged, this, &plQtGameObjectWidget::OnFilterTextChanged);

  layout()->addWidget(m_pFilterWidget);

  m_pTreeWidget = new plQtDocumentTreeView(this, pDocument, std::move(pCustomModel), pSelection);
  m_pTreeWidget->SetAllowDragDrop(true);
  m_pTreeWidget->SetAllowDeleteObjects(true);
  layout()->addWidget(m_pTreeWidget);

  m_pDocument->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plQtGameObjectWidget::DocumentSceneEventHandler, this));

  m_pTreeWidget->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

  PL_VERIFY(connect(m_pTreeWidget, SIGNAL(doubleClicked(const QModelIndex&)), this, SLOT(OnItemDoubleClicked(const QModelIndex&))) != nullptr,
    "signal/slot connection failed");
  PL_VERIFY(connect(m_pTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(OnRequestContextMenu(QPoint))) != nullptr,
    "signal/slot connection failed");
}

plQtGameObjectWidget::~plQtGameObjectWidget()
{
  m_pDocument->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plQtGameObjectWidget::DocumentSceneEventHandler, this));
}


void plQtGameObjectWidget::DocumentSceneEventHandler(const plGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectEvent::Type::TriggerShowSelectionInScenegraph:
    {
      m_pTreeWidget->EnsureLastSelectedItemVisible();
    }
    break;

    default:
      break;
  }
}

void plQtGameObjectWidget::OnItemDoubleClicked(const QModelIndex&)
{
  m_pDocument->TriggerFocusOnSelection(true);
}

void plQtGameObjectWidget::OnRequestContextMenu(QPoint pos)
{
  plQtMenuActionMapView menu(nullptr);

  plActionContext context;
  context.m_sMapping = m_sContextMenuMapping;
  context.m_pDocument = m_pDocument;
  context.m_pWindow = this;
  menu.SetActionContext(context);

  menu.exec(m_pTreeWidget->mapToGlobal(pos));
}

void plQtGameObjectWidget::OnFilterTextChanged(const QString& text)
{
  m_pTreeWidget->GetProxyFilterModel()->SetFilterText(text);
}


//////////////////////////////////////////////////////////////////////////

plQtGameObjectPanel::plQtGameObjectPanel(
  QWidget* pParent, plGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<plQtDocumentTreeModel> pCustomModel)
  : plQtDocumentPanel(pParent, pDocument)
{
  setObjectName("ScenegraphPanel");
  setWindowTitle("Scenegraph");

  m_pMainWidget = new plQtGameObjectWidget(this, pDocument, szContextMenuMapping, std::move(pCustomModel));
  setWidget(m_pMainWidget);
}

plQtGameObjectPanel::~plQtGameObjectPanel() = default;
