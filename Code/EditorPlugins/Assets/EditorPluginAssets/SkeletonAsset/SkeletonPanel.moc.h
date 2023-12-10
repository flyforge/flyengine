#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>

class plSkeletonAssetDocument;
class QTreeView;
class plQtDocumentTreeView;
class plQtSearchWidget;

class plQtSkeletonPanel : public plQtDocumentPanel
{
  Q_OBJECT

public:
  plQtSkeletonPanel(QWidget* pParent, plSkeletonAssetDocument* pDocument);
  ~plQtSkeletonPanel();

private:
  plSkeletonAssetDocument* m_pSkeletonDocument = nullptr;
  QWidget* m_pMainWidget = nullptr;
  plQtDocumentTreeView* m_pTreeWidget = nullptr;
  plQtSearchWidget* m_pFilterWidget = nullptr;
};

