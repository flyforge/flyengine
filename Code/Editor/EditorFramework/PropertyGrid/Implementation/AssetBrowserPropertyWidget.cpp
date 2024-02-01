#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plQtAssetPropertyWidget::plQtAssetPropertyWidget()
  : plQtStandardPropertyWidget()
{
  m_uiThumbnailID = 0;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new plQtAssetLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_pWidget->m_pOwner = this;
  setFocusProxy(m_pWidget);

  PL_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  PL_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr,
    "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::InstantPopup);

  QMenu* pMenu = new QMenu();
  pMenu->setToolTipsVisible(true);
  m_pButton->setMenu(pMenu);

  connect(pMenu, &QMenu::aboutToShow, this, &plQtAssetPropertyWidget::OnShowMenu);

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);

  PL_VERIFY(connect(plQtImageCache::GetSingleton(), &plQtImageCache::ImageLoaded, this, &plQtAssetPropertyWidget::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  PL_VERIFY(
    connect(plQtImageCache::GetSingleton(), &plQtImageCache::ImageInvalidated, this, &plQtAssetPropertyWidget::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");
}

bool plQtAssetPropertyWidget::IsValidAssetType(const char* szAssetReference) const
{
  plAssetCurator::plLockedSubAsset pAsset;

  if (!plConversionUtils::IsStringUuid(szAssetReference))
  {
    pAsset = plAssetCurator::GetSingleton()->FindSubAsset(szAssetReference);

    if (pAsset == nullptr)
    {
      const plAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<plAssetBrowserAttribute>();

      // if this file type is on the asset whitelist for this asset type, let it through
      return plAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(pAssetAttribute->GetTypeFilter(), szAssetReference);
    }
  }
  else
  {
    const plUuid AssetGuid = plConversionUtils::ConvertStringToUuid(szAssetReference);

    pAsset = plAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);
  }

  // invalid asset in general
  if (pAsset == nullptr)
    return false;

  const plAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<plAssetBrowserAttribute>();

  if (plStringUtils::IsEqual(pAssetAttribute->GetTypeFilter(), ";;")) // empty type list -> allows everything
    return true;

  plStringBuilder sTypeFilter(";", pAsset->m_Data.m_sSubAssetsDocumentTypeName, ";");

  if (plStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr)
    return true;

  if (const plDocumentTypeDescriptor* pDesc = plDocumentManager::GetDescriptorForDocumentType(pAsset->m_Data.m_sSubAssetsDocumentTypeName))
  {
    for (const plString& comp : pDesc->m_CompatibleTypes)
    {
      sTypeFilter.Set(";", comp, ";");

      if (plStringUtils::FindSubString_NoCase(pAssetAttribute->GetTypeFilter(), sTypeFilter) != nullptr)
        return true;
    }
  }

  return false;
}

void plQtAssetPropertyWidget::OnInit()
{
  PL_ASSERT_DEV(m_pProp->GetAttributeByType<plAssetBrowserAttribute>() != nullptr, "plQtAssetPropertyWidget was created without a plAssetBrowserAttribute!");
}

void plQtAssetPropertyWidget::UpdateThumbnail(const plUuid& guid, const char* szThumbnailPath)
{
  if (IsUndead())
    return;

  const QPixmap* pThumbnailPixmap = nullptr;

  if (guid.IsValid())
  {
    plUInt64 uiUserData1, uiUserData2;
    m_AssetGuid.GetValues(uiUserData1, uiUserData2);

    const plAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<plAssetBrowserAttribute>();
    plStringBuilder sTypeFilter = pAssetAttribute->GetTypeFilter();
    sTypeFilter.Trim(" ;");

    pThumbnailPixmap = plQtImageCache::GetSingleton()->QueryPixmapForType(
      sTypeFilter, szThumbnailPath, QModelIndex(), QVariant(uiUserData1), QVariant(uiUserData2), &m_uiThumbnailID);
  }

  if (pThumbnailPixmap)
  {
    m_pButton->setIcon(QIcon(pThumbnailPixmap->scaledToWidth(16, Qt::TransformationMode::SmoothTransformation)));
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  }
  else
  {
    m_pButton->setIcon(QIcon());
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  }
}

void plQtAssetPropertyWidget::InternalSetValue(const plVariant& value)
{
  plQtScopedBlockSignals b(m_pWidget);
  plQtScopedBlockSignals b2(m_pButton);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    plStringBuilder sText = value.ConvertTo<plString>();
    m_AssetGuid = plUuid();
    plStringBuilder sThumbnailPath;

    if (plConversionUtils::IsStringUuid(sText))
    {
      if (!IsValidAssetType(sText))
      {
        m_uiThumbnailID = 0;

        m_pWidget->setText(plMakeQString(sText));
        
        m_pButton->setIcon(QIcon());
        m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);

        m_Pal.setColor(QPalette::Text, Qt::red);
        m_pWidget->setPalette(m_Pal);

        return;
      }

      plUuid newAssetGuid = plConversionUtils::ConvertStringToUuid(sText);

      // If this is a thumbnail or transform dependency, make sure the target is not in our inverse hull, i.e. we don't create a circular dependency.
      const plAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<plAssetBrowserAttribute>();
      if (pAssetAttribute->GetDependencyFlags().IsAnySet(plDependencyFlags::Thumbnail | plDependencyFlags::Transform))
      {
        plUuid documentGuid = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetGuid();
        plAssetCurator::plLockedSubAsset asset = plAssetCurator::GetSingleton()->GetSubAsset(documentGuid);
        if (asset.isValid())
        {
          plSet<plUuid> inverseHull;
          plAssetCurator::GetSingleton()->GenerateInverseTransitiveHull(asset->m_pAssetInfo, inverseHull, true, true);
          if (inverseHull.Contains(newAssetGuid))
          {
            plQtUiServices::GetSingleton()->MessageBoxWarning("This asset can't be used here, as that would create a circular dependency.");
            return;
          }
        }
      }

      m_AssetGuid = newAssetGuid;
      auto pAsset = plAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid);

      if (pAsset)
      {
        pAsset->GetSubAssetIdentifier(sText);

        sThumbnailPath = pAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAsset->m_pAssetInfo->m_Path, pAsset->m_Data.m_sName);
      }
      else
      {
        m_AssetGuid = plUuid();
      }
    }

    UpdateThumbnail(m_AssetGuid, sThumbnailPath);

    {
      const QColor validColor = plToQtColor(plColorScheme::LightUI(plColorScheme::Green));
      const QColor invalidColor = plToQtColor(plColorScheme::LightUI(plColorScheme::Red));

      m_Pal.setColor(QPalette::Text, m_AssetGuid.IsValid() ? validColor : invalidColor);
      m_pWidget->setPalette(m_Pal);

      if (m_AssetGuid.IsValid())
        m_pWidget->setToolTip(QStringLiteral("The selected file resolved to a valid asset GUID"));
      else
        m_pWidget->setToolTip(QStringLiteral("The selected file is not a valid asset"));
    }

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void plQtAssetPropertyWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  m_pWidget->setPalette(m_Pal);
  plQtStandardPropertyWidget::showEvent(event);
}

void plQtAssetPropertyWidget::FillAssetMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  const bool bAsset = m_AssetGuid.IsValid();
  menu.setDefaultAction(menu.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open Asset"), this, SLOT(OnOpenAssetDocument()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Select in Asset Browser"), this, SLOT(OnSelectInAssetBrowser()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnCopyAssetGuid()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Create New Asset"), this, SLOT(OnCreateNewAsset()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Delete.svg"), QLatin1String("Clear Asset Reference"), this, SLOT(OnClearReference()))->setEnabled(bAsset);
}

void plQtAssetPropertyWidget::on_TextFinished_triggered()
{
  plStringBuilder sText = m_pWidget->text().toUtf8().data();

  auto pAsset = plAssetCurator::GetSingleton()->FindSubAsset(sText);

  if (pAsset)
  {
    plConversionUtils::ToString(pAsset->m_Data.m_Guid, sText);
  }

  BroadcastValueChanged(sText.GetData());
}


void plQtAssetPropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void plQtAssetPropertyWidget::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const plUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  if (guid == m_AssetGuid)
  {
    UpdateThumbnail(guid, sPath.toUtf8().data());
  }
}


void plQtAssetPropertyWidget::ThumbnailInvalidated(QString sPath, plUInt32 uiImageID)
{
  if (m_uiThumbnailID == uiImageID)
  {
    UpdateThumbnail(plUuid(), "");
  }
}

void plQtAssetPropertyWidget::OnOpenAssetDocument()
{
  plQtEditorApp::GetSingleton()->OpenDocumentQueued(
    plAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath(), GetSelection()[0].m_pObject);
}

void plQtAssetPropertyWidget::OnSelectInAssetBrowser()
{
  plQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(m_AssetGuid);
  plQtAssetBrowserPanel::GetSingleton()->raise();
}

void plQtAssetPropertyWidget::OnOpenExplorer()
{
  plString sPath;

  if (m_AssetGuid.IsValid())
  {
    sPath = plAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
  }
  else
  {
    sPath = m_pWidget->text().toUtf8().data();
    if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      return;
  }

  plQtUiServices::OpenInExplorer(sPath, true);
}

void plQtAssetPropertyWidget::OnCopyAssetGuid()
{
  plStringBuilder sGuid;

  if (m_AssetGuid.IsValid())
  {
    plConversionUtils::ToString(m_AssetGuid, sGuid);
  }
  else
  {
    sGuid = m_pWidget->text().toUtf8().data();
    if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sGuid))
      return;
  }

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData* mimeData = new QMimeData();
  mimeData->setText(QString::fromUtf8(sGuid.GetData()));
  clipboard->setMimeData(mimeData);

  plQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(plFmt("Copied asset GUID: {}", sGuid), plTime::MakeFromSeconds(5));
}

void plQtAssetPropertyWidget::OnCreateNewAsset()
{
  plString sPath;

  // try to pick a good path
  {
    if (m_AssetGuid.IsValid())
    {
      sPath = plAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
    }
    else
    {
      sPath = m_pWidget->text().toUtf8().data();

      if (sPath.IsEmpty())
      {
        sPath = ":project/";
      }

      plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }
  }

  const plAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<plAssetBrowserAttribute>();
  plStringBuilder sTypeFilter = pAssetAttribute->GetTypeFilter();

  plHybridArray<plString, 4> allowedTypes;
  sTypeFilter.Split(false, allowedTypes, ";");

  plStringBuilder tmp;

  for (plString& type : allowedTypes)
  {
    tmp = type;
    tmp.Trim(" ");
    type = tmp;
  }

  struct info
  {
    plAssetDocumentManager* pAssetMan = nullptr;
    const plDocumentTypeDescriptor* pDocType = nullptr;
  };

  plMap<plString, info> typesToUse;

  {
    const plHybridArray<plDocumentManager*, 16>& managers = plDocumentManager::GetAllDocumentManagers();

    for (plDocumentManager* pMan : managers)
    {
      if (auto pAssetMan = plDynamicCast<plAssetDocumentManager*>(pMan))
      {
        plHybridArray<const plDocumentTypeDescriptor*, 4> documentTypes;
        pAssetMan->GetSupportedDocumentTypes(documentTypes);

        for (const plDocumentTypeDescriptor* pType : documentTypes)
        {
          if (allowedTypes.IndexOf(pType->m_sDocumentTypeName) == plInvalidIndex)
          {
            for (const plString& compType : pType->m_CompatibleTypes)
            {
              if (allowedTypes.IndexOf(compType) != plInvalidIndex)
                goto allowed;
            }

            continue;
          }

        allowed:

          auto& toUse = typesToUse[pType->m_sDocumentTypeName];

          toUse.pAssetMan = pAssetMan;
          toUse.pDocType = pType;
        }
      }
    }
  }

  if (typesToUse.IsEmpty())
    return;

  plStringBuilder sFilter;
  QString sSelectedFilter;

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    const plString sAssetType = ttu.pDocType->m_sDocumentTypeName;
    const plString sExtension = ttu.pDocType->m_sFileExtension;

    sFilter.AppendWithSeparator(";;", sAssetType, " (*.", sExtension, ")");

    if (sSelectedFilter.isEmpty())
    {
      sSelectedFilter = sExtension.GetData();
    }
  }


  plStringBuilder sOutput = sPath;
  {

    QString sStartDir = sOutput.GetFileDirectory().GetData(tmp);
    sOutput = QFileDialog::getSaveFileName(
      QApplication::activeWindow(), "Create Asset", sStartDir, sFilter.GetData(), &sSelectedFilter, QFileDialog::Option::DontResolveSymlinks)
                .toUtf8()
                .data();

    if (sOutput.IsEmpty())
      return;
  }

  sFilter = sOutput.GetFileExtension();

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    if (sFilter.IsEqual_NoCase(ttu.pDocType->m_sFileExtension))
    {
      plDocument* pDoc = nullptr;

      const plStatus res = ttu.pAssetMan->CreateDocument(
        ttu.pDocType->m_sDocumentTypeName, sOutput, pDoc, plDocumentFlags::RequestWindow | plDocumentFlags::AddToRecentFilesList);

      plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Creating the document failed.");

      if (res.m_Result.Succeeded())
      {
        pDoc->EnsureVisible();

        InternalSetValue(sOutput.GetData());
        on_TextFinished_triggered();
      }
      break;
    }
  }
}

void plQtAssetPropertyWidget::OnClearReference()
{
  InternalSetValue("");
  on_TextFinished_triggered();
}

void plQtAssetPropertyWidget::OnShowMenu()
{
  m_pButton->menu()->clear();
  FillAssetMenu(*m_pButton->menu());
}

void plQtAssetPropertyWidget::on_BrowseFile_clicked()
{
  plStringBuilder sFile = m_pWidget->text().toUtf8().data();
  const plAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<plAssetBrowserAttribute>();

  plQtAssetBrowserDlg dlg(this, m_AssetGuid, pAssetAttribute->GetTypeFilter());
  if (dlg.exec() == 0)
    return;

  plUuid assetGuid = dlg.GetSelectedAssetGuid();
  if (assetGuid.IsValid())
    plConversionUtils::ToString(assetGuid, sFile);

  if (sFile.IsEmpty())
  {
    sFile = dlg.GetSelectedAssetPathRelative();

    if (sFile.IsEmpty())
    {
      sFile = dlg.GetSelectedAssetPathAbsolute();

      plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
    }
  }

  if (sFile.IsEmpty())
    return;

  InternalSetValue(sFile.GetData());

  on_TextFinished_triggered();
}
