#include <TestFramework/TestFrameworkPCH.h>

#ifdef PLASMA_USE_QT

#  include <QStringBuilder>
#  include <TestFramework/Framework/Qt/qtLogMessageDock.h>
#  include <TestFramework/Framework/TestFramework.h>

////////////////////////////////////////////////////////////////////////
// plQtLogMessageDock public functions
////////////////////////////////////////////////////////////////////////

plQtLogMessageDock::plQtLogMessageDock(QObject* pParent, const plTestFrameworkResult* pResult)
{
  setupUi(this);
  m_pModel = new plQtLogMessageModel(this, pResult);
  ListView->setModel(m_pModel);
}

plQtLogMessageDock::~plQtLogMessageDock()
{
  ListView->setModel(nullptr);
  delete m_pModel;
  m_pModel = nullptr;
}

void plQtLogMessageDock::resetModel()
{
  m_pModel->resetModel();
}

void plQtLogMessageDock::currentTestResultChanged(const plTestResultData* pTestResult)
{
  m_pModel->currentTestResultChanged(pTestResult);
  ListView->scrollToBottom();
}

void plQtLogMessageDock::currentTestSelectionChanged(const plTestResultData* pTestResult)
{
  m_pModel->currentTestSelectionChanged(pTestResult);
  ListView->scrollTo(m_pModel->GetLastIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
  ListView->scrollTo(m_pModel->GetFirstIndexOfTestSelection(), QAbstractItemView::EnsureVisible);
}

////////////////////////////////////////////////////////////////////////
// plQtLogMessageModel public functions
////////////////////////////////////////////////////////////////////////

plQtLogMessageModel::plQtLogMessageModel(QObject* pParent, const plTestFrameworkResult* pResult)
  : QAbstractItemModel(pParent)
  , m_pTestResult(pResult)
{
}

plQtLogMessageModel::~plQtLogMessageModel() = default;

void plQtLogMessageModel::resetModel()
{
  beginResetModel();
  currentTestResultChanged(nullptr);
  endResetModel();
}

QModelIndex plQtLogMessageModel::GetFirstIndexOfTestSelection()
{
  if (m_pCurrentTestSelection == nullptr || m_pCurrentTestSelection->m_iFirstOutput == -1)
    return QModelIndex();

  plInt32 iEntries = (plInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((plInt32)m_VisibleEntries[i] >= m_pCurrentTestSelection->m_iFirstOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

QModelIndex plQtLogMessageModel::GetLastIndexOfTestSelection()
{
  if (m_pCurrentTestSelection == nullptr || m_pCurrentTestSelection->m_iLastOutput == -1)
    return QModelIndex();

  plInt32 iEntries = (plInt32)m_VisibleEntries.size();
  for (int i = 0; i < iEntries; ++i)
  {
    if ((plInt32)m_VisibleEntries[i] >= m_pCurrentTestSelection->m_iLastOutput)
      return index(i, 0);
  }
  return index(rowCount() - 1, 0);
}

void plQtLogMessageModel::currentTestResultChanged(const plTestResultData* pTestResult)
{
  UpdateVisibleEntries();
  currentTestSelectionChanged(pTestResult);
}

void plQtLogMessageModel::currentTestSelectionChanged(const plTestResultData* pTestResult)
{
  m_pCurrentTestSelection = pTestResult;
  if (m_pCurrentTestSelection != nullptr)
  {
    dataChanged(index(m_pCurrentTestSelection->m_iFirstOutput, 0), index(m_pCurrentTestSelection->m_iLastOutput, 0));
  }
}


////////////////////////////////////////////////////////////////////////
// plQtLogMessageModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant plQtLogMessageModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || m_pTestResult == nullptr || index.column() != 0)
    return QVariant();

  const plInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (plInt32)m_VisibleEntries.size())
    return QVariant();

  const plUInt32 uiLogIdx = m_VisibleEntries[iRow];
  const plUInt8 uiIndention = m_VisibleEntriesIndention[iRow];
  const plTestOutputMessage& Message = *m_pTestResult->GetOutputMessage(uiLogIdx);
  const plTestErrorMessage* pError = (Message.m_iErrorIndex != -1) ? m_pTestResult->GetErrorMessage(Message.m_iErrorIndex) : nullptr;
  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      if (pError != nullptr)
      {
        QString sBlockStart = QLatin1String("\n") % QString((uiIndention + 1) * 3, ' ');
        QString sBlockName =
          pError->m_sBlock.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Block: ") + QLatin1String(pError->m_sBlock.c_str()));
        QString sMessage =
          pError->m_sMessage.empty() ? QLatin1String("") : (sBlockStart % QLatin1String("Message: ") + QLatin1String(pError->m_sMessage.c_str()));
        QString sErrorMessage = QString(uiIndention * 3, ' ') % QString(Message.m_sMessage.c_str()) % sBlockName % sBlockStart %
                                QLatin1String("File: ") % QLatin1String(pError->m_sFile.c_str()) % sBlockStart % QLatin1String("Line: ") %
                                QString::number(pError->m_iLine) % sBlockStart % QLatin1String("Function: ") %
                                QLatin1String(pError->m_sFunction.c_str()) % sMessage;

        return sErrorMessage;
      }
      return QString(uiIndention * 3, ' ') + QString(Message.m_sMessage.c_str());
    }
    case Qt::ForegroundRole:
    {
      switch (Message.m_Type)
      {
        case plTestOutput::BeginBlock:
        case plTestOutput::Message:
          return QColor(Qt::yellow);
        case plTestOutput::Error:
          return QColor(Qt::red);
        case plTestOutput::Success:
          return QColor(Qt::green);
        case plTestOutput::Warning:
          return QColor(qRgb(255, 100, 0));
        case plTestOutput::StartOutput:
        case plTestOutput::EndBlock:
        case plTestOutput::ImportantInfo:
        case plTestOutput::Details:
        case plTestOutput::Duration:
        case plTestOutput::FinalResult:
          return QVariant();
        default:
          return QVariant();
      }
    }
    case Qt::BackgroundRole:
    {
      QPalette palette = QApplication::palette();
      if (m_pCurrentTestSelection != nullptr && m_pCurrentTestSelection->m_iFirstOutput != -1)
      {
        if (m_pCurrentTestSelection->m_iFirstOutput <= (plInt32)uiLogIdx && (plInt32)uiLogIdx <= m_pCurrentTestSelection->m_iLastOutput)
        {
          return palette.midlight().color();
        }
      }
      return palette.base().color();
    }

    default:
      return QVariant();
  }
}

Qt::ItemFlags plQtLogMessageModel::flags(const QModelIndex& index) const
{
  if (!index.isValid() || m_pTestResult == nullptr)
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant plQtLogMessageModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Log Entry");
    }
  }
  return QVariant();
}

QModelIndex plQtLogMessageModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn, iRow);
}

QModelIndex plQtLogMessageModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int plQtLogMessageModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid() || m_pTestResult == nullptr)
    return 0;

  return (int)m_VisibleEntries.size();
}

int plQtLogMessageModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


////////////////////////////////////////////////////////////////////////
// plQtLogMessageModel private functions
////////////////////////////////////////////////////////////////////////

void plQtLogMessageModel::UpdateVisibleEntries()
{
  m_VisibleEntries.clear();
  m_VisibleEntriesIndention.clear();
  if (m_pTestResult == nullptr)
    return;

  plUInt8 uiIndention = 0;
  plUInt32 uiEntries = m_pTestResult->GetOutputMessageCount();
  /// \todo filter out uninteresting messages
  for (plUInt32 i = 0; i < uiEntries; ++i)
  {
    plTestOutput::Enum Type = m_pTestResult->GetOutputMessage(i)->m_Type;
    if (Type == plTestOutput::BeginBlock)
      uiIndention++;
    if (Type == plTestOutput::EndBlock)
      uiIndention--;

    m_VisibleEntries.push_back(i);
    m_VisibleEntriesIndention.push_back(uiIndention);
  }
  beginResetModel();
  endResetModel();
}

#endif

PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtLogMessageDock);
