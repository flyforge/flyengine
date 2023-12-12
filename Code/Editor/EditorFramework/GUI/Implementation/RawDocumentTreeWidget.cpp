#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>

plQtDocumentTreeView::plQtDocumentTreeView(QWidget* parent)
  : plQtItemView<QTreeView>(parent)
{
}

plQtDocumentTreeView::plQtDocumentTreeView(QWidget* pParent, plDocument* pDocument, std::unique_ptr<plQtDocumentTreeModel> pModel, plSelectionManager* pSelection)
  : plQtItemView<QTreeView>(pParent)
{
  Initialize(pDocument, std::move(pModel), pSelection);
}

void plQtDocumentTreeView::Initialize(plDocument* pDocument, std::unique_ptr<plQtDocumentTreeModel> pModel, plSelectionManager* pSelection)
{
  m_pDocument = pDocument;
  m_pModel = std::move(pModel);
  m_pSelectionManager = pSelection;
  if (m_pSelectionManager == nullptr)
  {
    // If no selection manager is provided, fall back to the default selection.
    m_pSelectionManager = m_pDocument->GetSelectionManager();
  }

  m_pFilterModel.reset(new plQtTreeSearchFilterModel(this));
  m_pFilterModel->setSourceModel(m_pModel.get());

  setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
  setModel(m_pFilterModel.get());
  setDragEnabled(true);
  setAcceptDrops(true);
  setDropIndicatorShown(true);
  setHeaderHidden(true);
  setExpandsOnDoubleClick(true);
  setEditTriggers(QAbstractItemView::EditTrigger::EditKeyPressed);
  setUniformRowHeights(true);

  PLASMA_VERIFY(connect(selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
              SLOT(on_selectionChanged_triggered(const QItemSelection&, const QItemSelection&))) != nullptr,
    "signal/slot connection failed");
  m_pSelectionManager->m_Events.AddEventHandler(plMakeDelegate(&plQtDocumentTreeView::SelectionEventHandler, this));

  plSelectionManagerEvent e;
  e.m_pDocument = m_pDocument;
  e.m_pObject = nullptr;
  e.m_Type = plSelectionManagerEvent::Type::SelectionSet;
  SelectionEventHandler(e);
}

plQtDocumentTreeView::~plQtDocumentTreeView()
{
  m_pSelectionManager->m_Events.RemoveEventHandler(plMakeDelegate(&plQtDocumentTreeView::SelectionEventHandler, this));
}

void plQtDocumentTreeView::on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (m_bBlockSelectionSignal)
    return;

  QModelIndexList selection = selectionModel()->selectedIndexes();

  plDeque<const plDocumentObject*> sel;

  foreach (QModelIndex index, selection)
  {
    if (index.isValid())
    {
      index = m_pFilterModel->mapToSource(index);

      if (index.isValid())
        sel.PushBack((const plDocumentObject*)index.internalPointer());
    }
  }

  // TODO const cast
  ((plSelectionManager*)m_pSelectionManager)->SetSelection(sel);
}

void plQtDocumentTreeView::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  switch (e.m_Type)
  {
    case plSelectionManagerEvent::Type::SelectionCleared:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      selectionModel()->clear();
      m_bBlockSelectionSignal = false;
    }
    break;
    case plSelectionManagerEvent::Type::SelectionSet:
    case plSelectionManagerEvent::Type::ObjectAdded:
    case plSelectionManagerEvent::Type::ObjectRemoved:
    {
      // Can't block signals on selection model or view won't update.
      m_bBlockSelectionSignal = true;
      QItemSelection selection;
      QModelIndex currentIndex;
      for (const plDocumentObject* pObject : m_pSelectionManager->GetSelection())
      {
        currentIndex = m_pModel->ComputeModelIndex(pObject);
        currentIndex = m_pFilterModel->mapFromSource(currentIndex);

        if (currentIndex.isValid())
          selection.select(currentIndex, currentIndex);
      }
      if (currentIndex.isValid())
      {
        // We need to change the current index as well because the current index can trigger side effects. E.g. deleting the current index row triggers a selection change event.
        selectionModel()->setCurrentIndex(currentIndex, QItemSelectionModel::SelectCurrent);
      }
      selectionModel()
        ->select(selection, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows | QItemSelectionModel::NoUpdate);
      m_bBlockSelectionSignal = false;
    }
    break;
  }
}

void plQtDocumentTreeView::EnsureLastSelectedItemVisible()
{
  if (m_pSelectionManager->GetSelection().IsEmpty())
    return;

  const plDocumentObject* pObject = m_pSelectionManager->GetSelection().PeekBack();
  PLASMA_ASSERT_DEBUG(m_pModel->GetDocumentTree()->GetDocument() == pObject->GetDocumentObjectManager()->GetDocument(), "Selection is from a different document.");

  auto index = m_pModel->ComputeModelIndex(pObject);
  index = m_pFilterModel->mapFromSource(index);

  if (index.isValid())
    scrollTo(index, QAbstractItemView::EnsureVisible);
}

void plQtDocumentTreeView::SetAllowDragDrop(bool bAllow)
{
  m_pModel->SetAllowDragDrop(bAllow);
}

void plQtDocumentTreeView::SetAllowDeleteObjects(bool bAllow)
{
  m_bAllowDeleteObjects = bAllow;
}

void plQtDocumentTreeView::keyPressEvent(QKeyEvent* e)
{
  if (plQtProxy::TriggerDocumentAction(m_pDocument, e))
    return;

  if (e == QKeySequence::Delete)
  {
    if (m_bAllowDeleteObjects)
    {
      m_pDocument->DeleteSelectedObjects();
    }
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}
