#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>
#include <QModelIndex>

class plSelectionContext;
struct plSelectionManagerEvent;

class PL_EDITORFRAMEWORK_DLL plQtGameObjectReferencePropertyWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtGameObjectReferencePropertyWidget();

private Q_SLOTS:
  void on_PickObject_clicked();

protected slots:
  void on_customContextMenuRequested(const QPoint& pt);
  void OnSelectReferencedObject();
  void OnCopyReference();
  void OnClearReference();
  void OnPasteReference();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;
  void FillContextMenu(QMenu& menu);
  void PickObjectOverride(const plDocumentObject* pObject);
  void SetValue(const QString& sText);
  void ClearPicking();
  void SelectionManagerEventHandler(const plSelectionManagerEvent& e);
  virtual void showEvent(QShowEvent* event) override;

protected:
  QPalette m_Pal;
  QHBoxLayout* m_pLayout = nullptr;
  QLabel* m_pWidget = nullptr;
  QString m_sInternalValue;
  QToolButton* m_pButton = nullptr;
  plHybridArray<plSelectionContext*, 8> m_SelectionContextsToUnsubscribe;
};

