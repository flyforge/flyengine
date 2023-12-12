#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>

PLASMA_IMPLEMENT_SINGLETON(plQtAssetBrowserPanel);

plQtAssetBrowserPanel::plQtAssetBrowserPanel()
  : plQtApplicationPanel("Panel.AssetBrowser")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  setWidget(pDummy);

  setIcon(plQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/AssetCurator.svg"));
  setWindowTitle(QString::fromUtf8(plTranslate("Panel.AssetBrowser")));

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

void plQtAssetBrowserPanel::SlotAssetChosen(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  plQtEditorApp::GetSingleton()->OpenDocumentQueued(sAssetPathAbsolute.toUtf8().data());
}

void plQtAssetBrowserPanel::SlotAssetSelected(plUuid guid, QString sAssetPathRelative, QString sAssetPathAbsolute)
{
  m_LastSelected = guid;
}

void plQtAssetBrowserPanel::SlotAssetCleared()
{
  m_LastSelected.SetInvalid();
}
