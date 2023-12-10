#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/Widgets/ItemView.moc.h>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <ToolsFoundation/Selection/SelectionManager.h>
#include <memory>

class plQtTreeSearchFilterModel;
class plSelectionManager;

class PLASMA_EDITORFRAMEWORK_DLL plQtDocumentTreeView : public plQtItemView<QTreeView>
{
  Q_OBJECT

public:
  plQtDocumentTreeView(QWidget* pParent);
  plQtDocumentTreeView(QWidget* pParent, plDocument* pDocument, std::unique_ptr<plQtDocumentTreeModel> pCustomModel, plSelectionManager* pSelection = nullptr);
  ~plQtDocumentTreeView();

  void Initialize(plDocument* pDocument, std::unique_ptr<plQtDocumentTreeModel> pCustomModel, plSelectionManager* pSelection = nullptr);

  void EnsureLastSelectedItemVisible();

  void SetAllowDragDrop(bool bAllow);
  void SetAllowDeleteObjects(bool bAllow);

  plQtTreeSearchFilterModel* GetProxyFilterModel() const { return m_pFilterModel.get(); }

protected:
  virtual bool event(QEvent* pEvent) override;

private Q_SLOTS:
  void on_selectionChanged_triggered(const QItemSelection& selected, const QItemSelection& deselected);

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);

private:
  std::unique_ptr<plQtDocumentTreeModel> m_pModel;
  std::unique_ptr<plQtTreeSearchFilterModel> m_pFilterModel;
  plSelectionManager* m_pSelectionManager = nullptr;
  plDocument* m_pDocument = nullptr;
  bool m_bBlockSelectionSignal = false;
  bool m_bAllowDeleteObjects = false;
};

