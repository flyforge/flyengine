#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QDockWidget>

class plDocument;

class PL_GUIFOUNDATION_DLL plQtDocumentPanel : public QDockWidget
{
public:
  Q_OBJECT

public:
  plQtDocumentPanel(QWidget* pParent, plDocument* pDocument);
  ~plQtDocumentPanel();

  // prevents closing of the dockwidget, even with Alt+F4
  virtual void closeEvent(QCloseEvent* e) override;
  virtual bool event(QEvent* pEvent) override;

  static const plDynamicArray<plQtDocumentPanel*>& GetAllDocumentPanels() { return s_AllDocumentPanels; }

private:
  plDocument* m_pDocument = nullptr;

  static plDynamicArray<plQtDocumentPanel*> s_AllDocumentPanels;
};

