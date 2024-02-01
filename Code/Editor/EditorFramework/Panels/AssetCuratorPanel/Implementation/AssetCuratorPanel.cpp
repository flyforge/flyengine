#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>

plQtAssetCuratorFilter::plQtAssetCuratorFilter(QObject* pParent)
  : plQtAssetFilter(pParent)
{
}

void plQtAssetCuratorFilter::SetFilterTransitive(bool bFilterTransitive)
{
  m_bFilterTransitive = bFilterTransitive;
}

bool plQtAssetCuratorFilter::IsAssetFiltered(plStringView sDataDirParentRelativePath, bool bIsFolder, const plSubAsset* pInfo) const
{
  if (!pInfo)
    return true;

  if (!pInfo->m_bMainAsset)
    return true;

  if (pInfo->m_pAssetInfo->m_TransformState != plAssetInfo::MissingTransformDependency && pInfo->m_pAssetInfo->m_TransformState != plAssetInfo::CircularDependency &&
      pInfo->m_pAssetInfo->m_TransformState != plAssetInfo::MissingThumbnailDependency && pInfo->m_pAssetInfo->m_TransformState != plAssetInfo::TransformError)
  {
    return true;
  }

  if (m_bFilterTransitive)
  {
    if (pInfo->m_pAssetInfo->m_TransformState == plAssetInfo::MissingThumbnailDependency)
    {
      for (auto& ref : pInfo->m_pAssetInfo->m_MissingThumbnailDeps)
      {
        if (!plAssetCurator::GetSingleton()->FindSubAsset(ref).isValid())
        {
          return false;
        }
      }

      return true;
    }
  }

  return false;
}

PL_IMPLEMENT_SINGLETON(plQtAssetCuratorPanel);

plQtAssetCuratorPanel::plQtAssetCuratorPanel()
  : plQtApplicationPanel("Panel.AssetCurator")
  , m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  // using pDummy instead of 'this' breaks auto-connect for slots
  setWidget(pDummy);
  setIcon(plQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/AssetCurator.svg"));
  setWindowTitle(plMakeQString(plTranslate("Panel.AssetCurator")));

  connect(ListAssets, &QTreeView::doubleClicked, this, &plQtAssetCuratorPanel::onListAssetsDoubleClicked);
  connect(CheckIndirect, &QCheckBox::toggled, this, &plQtAssetCuratorPanel::onCheckIndirectToggled);

  plAssetProcessor::GetSingleton()->AddLogWriter(plMakeDelegate(&plQtAssetCuratorPanel::LogWriter, this));

  m_pFilter = new plQtAssetCuratorFilter(this);
  m_pModel = new plQtAssetBrowserModel(this, m_pFilter);
  m_pModel->SetIconMode(false);

  TransformLog->ShowControls(false);

  ListAssets->setModel(m_pModel);
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  PL_VERIFY(
    connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this, &plQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr,
    "signal/slot connection failed");
  PL_VERIFY(connect(m_pModel, &QAbstractItemModel::dataChanged, this,
              [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
              {
                if (m_SelectedIndex.isValid() && topLeft.row() <= m_SelectedIndex.row() && m_SelectedIndex.row() <= bottomRight.row())
                {
                  UpdateIssueInfo();
                }
              }),
    "signal/slot connection failed");

  PL_VERIFY(connect(m_pModel, &QAbstractItemModel::modelReset, this,
              [this]()
              {
                m_SelectedIndex = QPersistentModelIndex();
                UpdateIssueInfo();
              }),
    "signal/slot connection failed");
}

plQtAssetCuratorPanel::~plQtAssetCuratorPanel()
{
  plAssetProcessor::GetSingleton()->RemoveLogWriter(plMakeDelegate(&plQtAssetCuratorPanel::LogWriter, this));
}

void plQtAssetCuratorPanel::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (selected.isEmpty())
    m_SelectedIndex = QModelIndex();
  else
    m_SelectedIndex = selected.indexes()[0];

  UpdateIssueInfo();
}

void plQtAssetCuratorPanel::onListAssetsDoubleClicked(const QModelIndex& index)
{
  QString sAbsPath = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

  plQtEditorApp::GetSingleton()->OpenDocumentQueued(sAbsPath.toUtf8().data());
}

void plQtAssetCuratorPanel::onCheckIndirectToggled(bool checked)
{
  m_pFilter->SetFilterTransitive(!checked);
  m_pModel->resetModel();
}

void plQtAssetCuratorPanel::LogWriter(const plLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  plLogEntry msg(e);
  CuratorLog->GetLog()->AddLogMsg(msg);
}

void plQtAssetCuratorPanel::UpdateIssueInfo()
{
  if (!m_SelectedIndex.isValid())
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  plUuid assetGuid = m_pModel->data(m_SelectedIndex, plQtAssetBrowserModel::UserRoles::AssetGuid).value<plUuid>();
  auto pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (pSubAsset == nullptr)
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  TransformLog->GetLog()->Clear();

  plAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;

  auto getNiceName = [](const plString& sDep) -> plStringBuilder
  {
    if (plConversionUtils::IsStringUuid(sDep))
    {
      plUuid guid = plConversionUtils::ConvertStringToUuid(sDep);
      auto assetInfoDep = plAssetCurator::GetSingleton()->GetSubAsset(guid);
      if (assetInfoDep)
      {
        return assetInfoDep->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      }

      plUInt64 uiLow;
      plUInt64 uiHigh;
      guid.GetValues(uiLow, uiHigh);
      plStringBuilder sTmp;
      sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);

      return sTmp;
    }

    return sDep;
  };

  plLogEntryDelegate logger(([this](plLogEntry& ref_entry) -> void
    { TransformLog->GetLog()->AddLogMsg(std::move(ref_entry)); }));
  plStringBuilder text;
  if (pAssetInfo->m_TransformState == plAssetInfo::MissingTransformDependency)
  {
    plLog::Error(&logger, "Missing Transform Dependency:");
    for (const plString& dep : pAssetInfo->m_MissingTransformDeps)
    {
      plStringBuilder m_sNiceName = getNiceName(dep);
      plLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == plAssetInfo::CircularDependency)
  {
    plLog::Error(&logger, "Circular Dependency:");
    for (const plString& ref : pAssetInfo->m_CircularDependencies)
    {
      plStringBuilder m_sNiceName = getNiceName(ref);
      plLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == plAssetInfo::MissingThumbnailDependency)
  {
    plLog::Error(&logger, "Missing Thumbnail Dependency:");
    for (const plString& ref : pAssetInfo->m_MissingThumbnailDeps)
    {
      plStringBuilder m_sNiceName = getNiceName(ref);
      plLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == plAssetInfo::TransformError)
  {
    plLog::Error(&logger, "Transform Error:");
    for (const plLogEntry& logEntry : pAssetInfo->m_LogEntries)
    {
      TransformLog->GetLog()->AddLogMsg(logEntry);
    }
  }
}
