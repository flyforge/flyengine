#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_AssetBrowserDlg.h>
#include <QDialog>

class plQtAssetBrowserDlg : public QDialog, public Ui_AssetBrowserDlg
{
  Q_OBJECT

public:
  plQtAssetBrowserDlg(QWidget* pParent, const plUuid& preselectedAsset, plStringView sVisibleFilters);
  plQtAssetBrowserDlg(QWidget* pParent, plStringView sWindowTitle, plStringView sPreselectedFileAbs, plStringView sFileExtensions);
  ~plQtAssetBrowserDlg();

  plStringView GetSelectedAssetPathRelative() const { return m_sSelectedAssetPathRelative; }
  plStringView GetSelectedAssetPathAbsolute() const { return m_sSelectedAssetPathAbsolute; }
  const plUuid GetSelectedAssetGuid() const { return m_SelectedAssetGuid; }

private Q_SLOTS:
  void on_AssetBrowserWidget_ItemChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags);
  void on_AssetBrowserWidget_ItemCleared();
  void on_ButtonSelect_clicked();

private:
  void Init(QWidget* pParent);

  plString m_sSelectedAssetPathRelative;
  plString m_sSelectedAssetPathAbsolute;
  plUuid m_SelectedAssetGuid;
  plString m_sVisibleFilters;

  static bool s_bShowItemsInSubFolder;
  static bool s_bShowItemsInHiddenFolder;
  static bool s_bSortByRecentUse;
  static plMap<plString, plString> s_TextFilter;
  static plMap<plString, plString> s_PathFilter;
  static plMap<plString, plString> s_TypeFilter;
};

