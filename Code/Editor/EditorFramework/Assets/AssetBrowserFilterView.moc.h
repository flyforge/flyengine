#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserFilter.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <QtWidgets/QTreeWidget>

class plQtAssetBrowserFilter;


class plQtAssetBrowserFilterView : public QWidget, public Ui_AssetBrowserFilter
{
  Q_OBJECT
public:
  plQtAssetBrowserFilterView(QWidget* parent);
  ~plQtAssetBrowserFilterView();
  
  

private:
  void AddPart(QTreeWidgetItem* node);
  void AddRoot();


public Q_SLOTS:
  void Reset();

public:
  void SetFilter(plQtAssetBrowserFilter* filter);
  void SetTree(QTreeWidget* tree) { TreeFolderFilter = tree; };
  

private:
  plDynamicArray<QWidget*> m_pParts;
  plUInt8 m_uPartCount; //max path filter depth is 255 elements
  plQtAssetBrowserFilter* m_pFilter;
  QTreeWidget* TreeFolderFilter;
};
