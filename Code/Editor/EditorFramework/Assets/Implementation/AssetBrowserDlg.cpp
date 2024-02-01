#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

bool plQtAssetBrowserDlg::s_bShowItemsInSubFolder = true;
bool plQtAssetBrowserDlg::s_bShowItemsInHiddenFolder = false;
bool plQtAssetBrowserDlg::s_bSortByRecentUse = true;
plMap<plString, plString> plQtAssetBrowserDlg::s_TextFilter;
plMap<plString, plString> plQtAssetBrowserDlg::s_PathFilter;
plMap<plString, plString> plQtAssetBrowserDlg::s_TypeFilter;

void plQtAssetBrowserDlg::Init(QWidget* pParent)
{
  setupUi(this);

  ButtonSelect->setEnabled(false);

  QSettings Settings;
  Settings.beginGroup(QLatin1String("AssetBrowserDlg"));
  {
    restoreGeometry(Settings.value("WindowGeometry", saveGeometry()).toByteArray());
    move(Settings.value("WindowPosition", pos()).toPoint());
    resize(Settings.value("WindowSize", size()).toSize());
  }
  Settings.endGroup();

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
}

plQtAssetBrowserDlg::plQtAssetBrowserDlg(QWidget* pParent, const plUuid& preselectedAsset, plStringView sVisibleFilters)
  : QDialog(pParent)
{
  {
    plStringBuilder temp = sVisibleFilters;
    plHybridArray<plStringView, 4> compTypes;
    temp.Split(false, compTypes, ";");
    plStringBuilder allFiltered = sVisibleFilters;

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
  Init(pParent);

  AssetBrowserWidget->SetMode(plQtAssetBrowserWidget::Mode::AssetPicker);

  if (m_sVisibleFilters != ";;") // that's an empty filter list
  {
    AssetBrowserWidget->ShowOnlyTheseTypeFilters(m_sVisibleFilters);
  }

  AssetBrowserWidget->SetSelectedAsset(preselectedAsset);

  AssetBrowserWidget->SearchWidget->setFocus();
}

plQtAssetBrowserDlg::plQtAssetBrowserDlg(QWidget* pParent, plStringView sWindowTitle, plStringView sPreselectedFileAbs, plStringView sFileExtensions)
  : QDialog(pParent)
{
  m_sVisibleFilters = sFileExtensions;

  Init(pParent);

  plStringBuilder title(sFileExtensions, ")");
  title.ReplaceAll(";", "; ");
  title.ReplaceAll("  ", " ");
  title.PrependFormat("{} (", sWindowTitle);
  setWindowTitle(plMakeQString(title));

  AssetBrowserWidget->SetMode(plQtAssetBrowserWidget::Mode::FilePicker);
  AssetBrowserWidget->UseFileExtensionFilters(sFileExtensions);

  plStringBuilder sParentRelPath = sPreselectedFileAbs;
  if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryParentRelative(sParentRelPath))
  {
    AssetBrowserWidget->GetAssetBrowserFilter()->SetTemporaryPinnedItem(sParentRelPath);
  }

  AssetBrowserWidget->SetSelectedFile(sPreselectedFileAbs);

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

void plQtAssetBrowserDlg::on_AssetBrowserWidget_ItemSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags)
{
  m_SelectedAssetGuid = guid;
  m_sSelectedAssetPathRelative = sAssetPathRelative.toUtf8().data();
  m_sSelectedAssetPathAbsolute = sAssetPathAbsolute.toUtf8().data();

  const plBitflags<plAssetBrowserItemFlags> flags = (plAssetBrowserItemFlags::Enum)uiAssetBrowserItemFlags;

  ButtonSelect->setEnabled(flags.IsAnySet(plAssetBrowserItemFlags::Asset | plAssetBrowserItemFlags::SubAsset | plAssetBrowserItemFlags::File));
}

void plQtAssetBrowserDlg::on_AssetBrowserWidget_ItemChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags)
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

void plQtAssetBrowserDlg::on_ButtonSelect_clicked()
{
  accept();
}
