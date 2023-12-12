#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Assets/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>

plQtAssetCuratorFilter::plQtAssetCuratorFilter(QObject* pParent)
  : plQtAssetFilter(pParent)
{
}

void plQtAssetCuratorFilter::SetFilterTransitive(bool bFilterTransitive)
{
  m_bFilterTransitive = bFilterTransitive;
}

bool plQtAssetCuratorFilter::IsAssetFiltered(const plSubAsset* pInfo) const
{
  if (!pInfo->m_bMainAsset)
    return true;

  if (pInfo->m_pAssetInfo->m_TransformState != plAssetInfo::MissingDependency &&
      pInfo->m_pAssetInfo->m_TransformState != plAssetInfo::MissingReference && pInfo->m_pAssetInfo->m_TransformState != plAssetInfo::TransformError)
  {
    return true;
  }

  if (m_bFilterTransitive)
  {
    if (pInfo->m_pAssetInfo->m_TransformState == plAssetInfo::MissingReference)
    {
      for (auto& ref : pInfo->m_pAssetInfo->m_MissingReferences)
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

bool plQtAssetCuratorFilter::Less(const plSubAsset* pInfoA, const plSubAsset* pInfoB) const
{
  // TODO: We can't sort on mutable data here as it destroys the set order, need to add a sorting model on top.
  // if (pInfoA->m_pAssetInfo->m_TransformState != pInfoB->m_pAssetInfo->m_TransformState)
  //  return pInfoA->m_pAssetInfo->m_TransformState < pInfoB->m_pAssetInfo->m_TransformState;

  plStringView sSortA = pInfoA->GetName();
  plStringView sSortB = pInfoB->GetName();

  plInt32 iValue = plStringUtils::Compare_NoCase(sSortA.GetStartPointer(), sSortB.GetStartPointer(), sSortA.GetEndPointer(), sSortB.GetEndPointer());
  if (iValue == 0)
  {
    return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
  }
  return iValue < 0;
}


plQtAssetCuratorPanel::plQtAssetCuratorPanel(QWidget * parent)
  : QWidget(parent)
{
  setupUi(this);

  connect(ListAssets, &QTreeView::doubleClicked, this, &plQtAssetCuratorPanel::onListAssetsDoubleClicked);
  connect(CheckIndirect, &QCheckBox::toggled, this, &plQtAssetCuratorPanel::onCheckIndirectToggled);

  plAssetProcessor::GetSingleton()->AddLogWriter(plMakeDelegate(&plQtAssetCuratorPanel::LogWriter, this));

  m_pFilter = new plQtAssetCuratorFilter(this);
  m_pModel = new plQtAssetBrowserModel(this, m_pFilter);
  m_pModel->SetIconMode(false);

  TransformLog->ShowControls(false);

  ListAssets->setModel(m_pModel);
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  PLASMA_VERIFY(
    connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this, &plQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr,
    "signal/slot connection failed");
  PLASMA_VERIFY(connect(m_pModel, &QAbstractItemModel::dataChanged, this,
              [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
                if (m_SelectedIndex.isValid() && topLeft.row() <= m_SelectedIndex.row() && m_SelectedIndex.row() <= bottomRight.row())
                {
                  UpdateIssueInfo();
                }
              }),
    "signal/slot connection failed");

  PLASMA_VERIFY(connect(m_pModel, &QAbstractItemModel::modelReset, this,
              [this]() {
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
  plUuid guid = m_pModel->data(index, plQtAssetBrowserModel::UserRoles::SubAssetGuid).value<plUuid>();
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

  auto getNiceName = [](const plString& dep) -> plStringBuilder {
    if (plConversionUtils::IsStringUuid(dep))
    {
      plUuid guid = plConversionUtils::ConvertStringToUuid(dep);
      auto assetInfoDep = plAssetCurator::GetSingleton()->GetSubAsset(guid);
      if (assetInfoDep)
      {
        return assetInfoDep->m_pAssetInfo->m_sDataDirParentRelativePath;
      }

      plUInt64 uiLow;
      plUInt64 uiHigh;
      guid.GetValues(uiLow, uiHigh);
      plStringBuilder sTmp;
      sTmp.Format("{} - u4{{},{}}", dep, uiLow, uiHigh);

      return sTmp;
    }

    return dep;
  };

  plLogEntryDelegate logger(([this](plLogEntry& entry) -> void { TransformLog->GetLog()->AddLogMsg(std::move(entry)); }));
  plStringBuilder text;
  if (pAssetInfo->m_TransformState == plAssetInfo::MissingDependency)
  {
    plLog::Error(&logger, "Missing Dependency:");
    for (const plString& dep : pAssetInfo->m_MissingDependencies)
    {
      plStringBuilder sNiceName = getNiceName(dep);
      plLog::Error(&logger, "{0}", sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == plAssetInfo::MissingReference)
  {
    plLog::Error(&logger, "Missing Reference:");
    for (const plString& ref : pAssetInfo->m_MissingReferences)
    {
      plStringBuilder sNiceName = getNiceName(ref);
      plLog::Error(&logger, "{0}", sNiceName);
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
