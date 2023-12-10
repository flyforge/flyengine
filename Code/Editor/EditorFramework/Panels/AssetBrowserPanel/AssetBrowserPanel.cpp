#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>

PLASMA_IMPLEMENT_SINGLETON(plQtAssetBrowserPanel);

plQtAssetBrowserPanel::plQtAssetBrowserPanel()
  : plQtApplicationPanel("Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  m_pStatusBar = new QStatusBar(nullptr);
  m_pStatusBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  m_pStatusBar->setSizeGripEnabled(false);

  m_pCuratorControl = new plQtCuratorControl(nullptr);

  m_pStatusBar->addPermanentWidget(m_pCuratorControl);

  dockWidgetContents->layout()->addWidget(m_pStatusBar);
  setWidget(pDummy);

  setIcon(plQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/Asset.svg"));
  setWindowTitle(plMakeQString(plTranslate("Panel.AssetBrowser")));

  PLASMA_VERIFY(connect(AssetBrowserWidget, &plQtAssetBrowserWidget::ItemChosen, this, &plQtAssetBrowserPanel::SlotAssetChosen) != nullptr,
    "signal/slot connection failed");
  PLASMA_VERIFY(connect(AssetBrowserWidget, &plQtAssetBrowserWidget::ItemSelected, this, &plQtAssetBrowserPanel::SlotAssetSelected) != nullptr,
    "signal/slot connection failed");
  PLASMA_VERIFY(connect(AssetBrowserWidget, &plQtAssetBrowserWidget::ItemCleared, this, &plQtAssetBrowserPanel::SlotAssetCleared) != nullptr,
    "signal/slot connection failed");

  AssetBrowserWidget->RestoreState("AssetBrowserPanel2");
}

plQtAssetBrowserPanel::~plQtAssetBrowserPanel()
{
  AssetBrowserWidget->SaveState("AssetBrowserPanel2");
}

void plQtAssetBrowserPanel::SlotAssetChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags)
{
  if (guid.IsValid())
  {
    plQtEditorApp::GetSingleton()->OpenDocumentQueued(sAssetPathAbsolute.toUtf8().data());
  }
  else
  {
    plQtUiServices::OpenFileInDefaultProgram(qtToPlasmaString(sAssetPathAbsolute));
  }
}

void plQtAssetBrowserPanel::SlotAssetSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute, plUInt8 uiAssetBrowserItemFlags)
{
  m_LastSelected = guid;
}

void plQtAssetBrowserPanel::SlotAssetCleared()
{
  m_LastSelected = plUuid::MakeInvalid();
}
