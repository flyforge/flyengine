#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/GUI/RawDocumentTreeWidget.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class plQtSearchWidget;
class plGameObjectDocument;
struct plGameObjectEvent;

class PLASMA_EDITORFRAMEWORK_DLL plQtGameObjectWidget : public QWidget
{
  Q_OBJECT

public:
  plQtGameObjectWidget(
    QWidget* pParent, plGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<plQtDocumentTreeModel> pCustomModel, plSelectionManager* pSelection = nullptr);
  ~plQtGameObjectWidget();

private Q_SLOTS:
  void OnItemDoubleClicked(const QModelIndex&);
  void OnRequestContextMenu(QPoint pos);
  void OnFilterTextChanged(const QString& text);

private:
  void DocumentSceneEventHandler(const plGameObjectEvent& e);

protected:
  plGameObjectDocument* m_pDocument;
  plQtDocumentTreeView* m_pTreeWidget;
  plQtSearchWidget* m_pFilterWidget;
  plString m_sContextMenuMapping;
};

class PLASMA_EDITORFRAMEWORK_DLL plQtGameObjectPanel : public plQtDocumentPanel
{
  Q_OBJECT

public:
  plQtGameObjectPanel(
    QWidget* pParent, plGameObjectDocument* pDocument, const char* szContextMenuMapping, std::unique_ptr<plQtDocumentTreeModel> pCustomModel);
  ~plQtGameObjectPanel();


protected:
  plQtGameObjectWidget* m_pMainWidget = nullptr;
};
