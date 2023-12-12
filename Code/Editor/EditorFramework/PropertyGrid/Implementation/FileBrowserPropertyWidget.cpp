#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>


plQtFilePropertyWidget::plQtFilePropertyWidget()
  : plQtStandardPropertyWidget()
{
  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  setLayout(m_pLayout);

  m_pWidget = new QLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  setFocusProxy(m_pWidget);

  PLASMA_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  PLASMA_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  m_pButton->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);

  {
    QMenu* pMenu = new QMenu();

    pMenu->setDefaultAction(pMenu->addAction(QIcon(), QLatin1String("Select File"), this, SLOT(on_BrowseFile_clicked())));
    QAction* pDocAction = pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open File"), this, SLOT(OnOpenFile())) /*->setEnabled(!m_pWidget->text().isEmpty())*/;
    pMenu->addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()));

    connect(pMenu, &QMenu::aboutToShow, pMenu, [=]() { pDocAction->setEnabled(!m_pWidget->text().isEmpty()); });

    m_pButton->setMenu(pMenu);
  }

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);
}

void plQtFilePropertyWidget::OnInit()
{
  auto pAttr = m_pProp->GetAttributeByType<plFileBrowserAttribute>();
  PLASMA_ASSERT_DEV(pAttr != nullptr, "plQtFilePropertyWidget was created without a plFileBrowserAttribute!");

  if (!plStringUtils::IsNullOrEmpty(pAttr->GetCustomAction()))
  {
    m_pButton->menu()->addAction(QIcon(), plTranslate(pAttr->GetCustomAction()), this, SLOT(OnCustomAction()));
  }
}

void plQtFilePropertyWidget::InternalSetValue(const plVariant& value)
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

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void plQtFilePropertyWidget::on_TextFinished_triggered()
{
  plStringBuilder sText = m_pWidget->text().toUtf8().data();

  BroadcastValueChanged(sText.GetData());
}

void plQtFilePropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void plQtFilePropertyWidget::OnOpenExplorer()
{
  plString sPath = m_pWidget->text().toUtf8().data();
  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  plQtUiServices::OpenInExplorer(sPath, true);
}


void plQtFilePropertyWidget::OnCustomAction()
{
  auto pAttr = m_pProp->GetAttributeByType<plFileBrowserAttribute>();

  if (pAttr->GetCustomAction() == nullptr)
    return;

  auto it = plDocumentManager::s_CustomActions.Find(pAttr->GetCustomAction());

  if (!it.IsValid())
    return;

  plVariant res = it.Value()(m_pGrid->GetDocument());

  if (!res.IsValid() || !res.IsA<plString>())
    return;

  m_pWidget->setText(res.Get<plString>().GetData());
  on_TextFinished_triggered();
}

void plQtFilePropertyWidget::OnOpenFile()
{
  plString sPath = m_pWidget->text().toUtf8().data();
  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
    return;

  if (!plQtUiServices::OpenFileInDefaultProgram(sPath))
    plQtUiServices::MessageBoxInformation(plFmt("File could not be opened:\n{0}\nCheck that the file exists, that a program is associated "
                                                "with this file type and that access to this file is not denied.",
      sPath));
}

static plMap<plString, plString> s_StartDirs;

void plQtFilePropertyWidget::on_BrowseFile_clicked()
{
  plString sFile = m_pWidget->text().toUtf8().data();
  const plFileBrowserAttribute* pFileAttribute = m_pProp->GetAttributeByType<plFileBrowserAttribute>();

  auto& sStartDir = s_StartDirs[pFileAttribute->GetTypeFilter()];

  if (!sFile.IsEmpty())
  {
    plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sFile);

    plStringBuilder st = sFile;
    st = st.GetFileDirectory();

    sStartDir = st;
  }

  if (sStartDir.IsEmpty())
    sStartDir = plToolsProject::GetSingleton()->GetProjectFile();

  QString sResult = QFileDialog::getOpenFileName(this, pFileAttribute->GetDialogTitle(), sStartDir.GetData(), pFileAttribute->GetTypeFilter(), nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sResult.isEmpty())
    return;

  sFile = sResult.toUtf8().data();
  sStartDir = sFile;

  if (!plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile))
  {
    plQtUiServices::GetSingleton()->MessageBoxInformation("The selected file is not under any data directory.\nPlease select another file "
                                                          "or copy it into one of the project's data directories.");
    return;
  }

  m_pWidget->setText(sFile.GetData());
  on_TextFinished_triggered();
}
