#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>


bool plQtAssetBrowserDlg::s_bShowItemsInSubFolder = false;
bool plQtAssetBrowserDlg::s_bShowItemsInHiddenFolder = false;
bool plQtAssetBrowserDlg::s_bSortByRecentUse = true;
plMap<plString, plString> plQtAssetBrowserDlg::s_TextFilter;
plMap<plString, plString> plQtAssetBrowserDlg::s_PathFilter;
plMap<plString, plString> plQtAssetBrowserDlg::s_TypeFilter;

plQtAssetBrowserDlg::plQtAssetBrowserDlg(QWidget* parent, const plUuid& preselectedAsset, const char* szVisibleFilters)
  : QDialog(parent)
{
  setupUi(this);

  ButtonFileDialog->setVisible(false); // not working correctly anymore

  {
    plStringBuilder temp = szVisibleFilters;
    plHybridArray<plStringView, 4> compTypes;
    temp.Split(false, compTypes, ";");
    plStringBuilder allFiltered = szVisibleFilters;

    for (const auto& descIt : plAssetDocumentManager::GetAllDocumentDescriptors())
    {
      const plDocumentTypeDescriptor* pType = descIt.Value();
      for (plStringView ct : compTypes)
      {
        if (pType->m_CompatibleTypes.Contains(ct))
        {
          allFiltered.Append(";", pType->m_sDocumentTypeName, ";");
        }
      }
    }

    m_sVisibleFilters = allFiltered;
  }

  ButtonSelect->setEnabled(false);

  AssetBrowserWidget->SetSelectedAsset(preselectedAsset);

  if (!plStringUtils::IsEqual(m_sVisibleFilters, ";;")) // that's an empty filter list
  {
    AssetBrowserWidget->ShowOnlyTheseTypeFilters(m_sVisibleFilters);
  }

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());
  }
  Settings.endGroup();

  AssetBrowserWidget->SetDialogMode();
  AssetBrowserWidget->RestoreState("AssetBrowserDlg");
  AssetBrowserWidget->GetAssetBrowserFilter()->SetSortByRecentUse(s_bSortByRecentUse);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInSubFolders(s_bShowItemsInSubFolder);
  AssetBrowserWidget->GetAssetBrowserFilter()->SetShowItemsInHiddenFolders(s_bShowItemsInHiddenFolder);

  if (!s_TextFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTextFilter(s_TextFilter[m_sVisibleFilters]);

  if (!s_PathFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetPathFilter(s_PathFilter[m_sVisibleFilters]);

  if (!s_TypeFilter[m_sVisibleFilters].IsEmpty())
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTypeFilter(s_TypeFilter[m_sVisibleFilters]);

  AssetBrowserWidget->SearchWidget->setFocus();
}

plQtAssetBrowserDlg::~plQtAssetBrowserDlg()
{
  s_bShowItemsInSubFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInSubFolders();
  s_bShowItemsInHiddenFolder = AssetBrowserWidget->GetAssetBrowserFilter()->GetShowItemsInHiddenFolders();
  s_bSortByRecentUse = AssetBrowserWidget->GetAssetBrowserFilter()->GetSortByRecentUse();
  s_TextFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTextFilter();
  s_PathFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetPathFilter();
  s_TypeFilter[m_sVisibleFilters] = AssetBrowserWidget->GetAssetBrowserFilter()->GetTypeFilter();

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    Settings.setValue("WindowGeometry", saveGeometry());
    Settings.setValue("WindowPosition", pos());
    Settings.setValue("WindowSize", size());
  }
  Settings.endGroup();

  AssetBrowserWidget->SaveState("AssetBrowserDlg");
}

void plQtAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  ButtonSelect->setEnabled(m_SelectedAssetGuid.IsValid());
}

void plQtAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  accept();
}

void plQtAssetBrowserDlg::on_AssetBrowserWidget_ItemCleared()
{
  ButtonSelect->setEnabled(false);
}

void plQtAssetBrowserDlg::on_ButtonFileDialog_clicked()
{
  hide();

  static QString sLastPath;

  m_SelectedAssetGuid = plUuid();
  m_sSelectedAssetPathRelative.Clear();
  m_sSelectedAssetPathAbsolute.Clear();

  const QString sFile = QFileDialog::getOpenFileName(
    QApplication::activeWindow(), QLatin1String("Open File"), sLastPath, QString(), nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
  {
    reject();
    return;
  }

  m_sSelectedAssetPathAbsolute = sFile.toUtf8().data();
  m_sSelectedAssetPathRelative = m_sSelectedAssetPathAbsolute;

  if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(m_sSelectedAssetPathRelative))
  {
    // \todo Message Box: Invalid Path

    // reject();
    // return;
  }

  sLastPath = sFile;
  on_AssetBrowserWidget_ItemChosen(plUuid(), QString::fromUtf8(m_sSelectedAssetPathRelative.GetData()), sFile);
}

void plQtAssetBrowserDlg::on_ButtonSelect_clicked()
{
  /// \todo Deactivate Ok button, when nothing is selectable

  accept();
}
