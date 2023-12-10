#include <TestFramework/TestFrameworkPCH.h>

#ifdef PLASMA_USE_QT

#  include <QApplication>
#  include <QPalette>
#  include <TestFramework/Framework/Qt/qtTestModel.h>

////////////////////////////////////////////////////////////////////////
// plQtTestModelEntry public functions
////////////////////////////////////////////////////////////////////////

plQtTestModelEntry::plQtTestModelEntry(const plTestFrameworkResult* pResult, plInt32 iTestIndex, plInt32 iSubTestIndex)
  : m_pResult(pResult)
  , m_iTestIndex(iTestIndex)
  , m_iSubTestIndex(iSubTestIndex)

{
}

plQtTestModelEntry::~plQtTestModelEntry()
{
  ClearEntries();
}

void plQtTestModelEntry::ClearEntries()
{
  for (plInt32 i = (plInt32)m_SubEntries.size() - 1; i >= 0; --i)
  {
    delete m_SubEntries[i];
  }
  m_SubEntries.clear();
}
plUInt32 plQtTestModelEntry::GetNumSubEntries() const

{
  return (plInt32)m_SubEntries.size();
}

plQtTestModelEntry* plQtTestModelEntry::GetSubEntry(plUInt32 uiIndex) const
{
  if (uiIndex >= GetNumSubEntries())
    return nullptr;

  return m_SubEntries[uiIndex];
}

void plQtTestModelEntry::AddSubEntry(plQtTestModelEntry* pEntry)
{
  pEntry->m_pParentEntry = this;
  pEntry->m_uiIndexInParent = (plUInt32)m_SubEntries.size();
  m_SubEntries.push_back(pEntry);
}

plQtTestModelEntry::plTestModelEntryType plQtTestModelEntry::GetNodeType() const
{
  return (m_iTestIndex == -1) ? RootNode : ((m_iSubTestIndex == -1) ? TestNode : SubTestNode);
}

const plTestResultData* plQtTestModelEntry::GetTestResult() const
{
  switch (GetNodeType())
  {
    case plQtTestModelEntry::TestNode:
    case plQtTestModelEntry::SubTestNode:
      return &m_pResult->GetTestResultData(m_iTestIndex, m_iSubTestIndex);
    default:
      return nullptr;
  }
}

static QColor ToneColor(const QColor& inputColor, const QColor& toneColor)
{
  qreal fHue = toneColor.hueF();
  qreal fSaturation = 1.0f;
  qreal fLightness = inputColor.lightnessF();
  fLightness = plMath::Clamp(fLightness, 0.20, 0.80);
  return QColor::fromHslF(fHue, fSaturation, fLightness);
}

////////////////////////////////////////////////////////////////////////
// plQtTestModel public functions
////////////////////////////////////////////////////////////////////////

plQtTestModel::plQtTestModel(QObject* pParent, plQtTestFramework* pTestFramework)
  : QAbstractItemModel(pParent)
  , m_pTestFramework(pTestFramework)
  , m_Root(nullptr)
{
  QPalette palette = QApplication::palette();
  m_pResult = &pTestFramework->GetTestResult();

  // Derive state colors from the current active palette.
  m_SucessColor = ToneColor(palette.text().color(), QColor(Qt::green)).toRgb();
  m_FailedColor = ToneColor(palette.text().color(), QColor(Qt::red)).toRgb();

  m_TestColor = ToneColor(palette.base().color(), QColor(Qt::cyan)).toRgb();
  m_SubTestColor = ToneColor(palette.base().color(), QColor(Qt::blue)).toRgb();

  m_TestIcon = QIcon(":/Icons/Icons/pie.png");
  m_TestIconOff = QIcon(":/Icons/Icons/pie_off.png");

  UpdateModel();
}

plQtTestModel::~plQtTestModel()
{
  m_Root.ClearEntries();
}

void plQtTestModel::Reset()
{
  beginResetModel();
  endResetModel();
}

void plQtTestModel::InvalidateAll()
{
  dataChanged(QModelIndex(), QModelIndex());
}

void plQtTestModel::TestDataChanged(plInt32 iTestIndex, plInt32 iSubTestIndex)
{
  QModelIndex TestModelIndex = index(iTestIndex, 0);
  // Invalidate whole test row
  Q_EMIT dataChanged(TestModelIndex, index(iTestIndex, columnCount() - 1));

  // Invalidate all sub-tests
  const plQtTestModelEntry* pEntry = (plQtTestModelEntry*)TestModelIndex.internalPointer();
  plInt32 iChildren = (plInt32)pEntry->GetNumSubEntries();
  Q_EMIT dataChanged(index(0, 0, TestModelIndex), index(iChildren - 1, columnCount() - 1, TestModelIndex));
}


////////////////////////////////////////////////////////////////////////
// plQtTestModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant plQtTestModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid())
    return QVariant();

  const plQtTestModelEntry* pEntry = (plQtTestModelEntry*)index.internalPointer();
  const plQtTestModelEntry* pParentEntry = pEntry->GetParentEntry();
  const plQtTestModelEntry::plTestModelEntryType entryType = pEntry->GetNodeType();

  bool bTestEnabled = true;
  bool bParentEnabled = true;
  bool bIsSubTest = entryType == plQtTestModelEntry::SubTestNode;
  const std::string& testUnavailableReason = m_pTestFramework->IsTestAvailable(bIsSubTest ? pParentEntry->GetTestIndex() : pEntry->GetTestIndex());

  if (bIsSubTest)
  {
    bTestEnabled = m_pTestFramework->IsSubTestEnabled(pEntry->GetTestIndex(), pEntry->GetSubTestIndex());
    bParentEnabled = m_pTestFramework->IsTestEnabled(pParentEntry->GetTestIndex());
  }
  else
  {
    bTestEnabled = m_pTestFramework->IsTestEnabled(pEntry->GetTestIndex());
  }

  const plTestResultData& TestResult = *pEntry->GetTestResult();

  // Name
  if (index.column() == Columns::Name)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QString(TestResult.m_sName.c_str());
      }
      case Qt::CheckStateRole:
      {
        return bTestEnabled ? Qt::Checked : Qt::Unchecked;
      }
      case Qt::DecorationRole:
      {
        return (bTestEnabled && bParentEnabled) ? m_TestIcon : m_TestIconOff;
      }
      case Qt::ForegroundRole:
      {
        if (!testUnavailableReason.empty())
        {
          QPalette palette = QApplication::palette();
          return palette.color(QPalette::Disabled, QPalette::Text);
        }
      }
      case Qt::ToolTipRole:
      {
        if (!testUnavailableReason.empty())
        {
          return QString("Test not available: %1").arg(testUnavailableReason.c_str());
        }
      }
      default:
        return QVariant();
    }
  }
  // Status
  else if (index.column() == Columns::Status)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        if (!testUnavailableReason.empty())
        {
          return QString("Test not available: %1").arg(testUnavailableReason.c_str());
        }
        else if (bTestEnabled && bParentEnabled)
        {
          if (bIsSubTest)
          {
            return QString("Enabled");
          }
          else
          {
            // Count sub-test status
            const plUInt32 iSubTests = m_pResult->GetSubTestCount(pEntry->GetTestIndex());
            const plUInt32 iEnabled = m_pTestFramework->GetSubTestEnabledCount(pEntry->GetTestIndex());

            if (iEnabled == iSubTests)
            {
              return QString("All Enabled");
            }

            return QString("%1 / %2 Enabled").arg(iEnabled).arg(iSubTests);
          }
        }
        else
        {
          return QString("Disabled");
        }
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }
      default:
        return QVariant();
    }
  }
  // Duration
  else if (index.column() == Columns::Duration)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QLocale(QLocale::English).toString(TestResult.m_fTestDuration, 'f', 4);
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }
      /*case Qt::BackgroundRole:
        {
          QPalette palette = QApplication::palette();
          return palette.alternateBase().color();
        }*/
      case UserRoles::Duration:
      {
        if (bIsSubTest && TestResult.m_bExecuted)
        {
          return TestResult.m_fTestDuration / pParentEntry->GetTestResult()->m_fTestDuration;
        }
        else if (TestResult.m_bExecuted)
        {
          return TestResult.m_fTestDuration / m_pTestFramework->GetTotalTestDuration();
        }
        return QVariant();
      }
      case UserRoles::DurationColor:
      {
        if (TestResult.m_bExecuted)
        {
          return (bIsSubTest ? m_SubTestColor : m_TestColor);
        }
        return QVariant();
      }
      default:
        return QVariant();
    }
  }
  // Errors
  else if (index.column() == Columns::Errors)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QString("%1 / %2")
          .arg(m_pResult->GetErrorMessageCount(pEntry->GetTestIndex(), pEntry->GetSubTestIndex()))
          .arg(m_pResult->GetOutputMessageCount(pEntry->GetTestIndex(), pEntry->GetSubTestIndex()));
      }
      case Qt::BackgroundRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
      case Qt::ForegroundRole:
      {
        if (TestResult.m_bExecuted)
        {
          return (m_pResult->GetErrorMessageCount(pEntry->GetTestIndex(), pEntry->GetSubTestIndex()) == 0) ? m_SucessColor : m_FailedColor;
        }
        return QVariant();
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

      default:
        return QVariant();
    }
  }
  // Assert Count
  else if (index.column() == Columns::Asserts)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return QString("%1").arg(TestResult.m_iTestAsserts);
      }
      case Qt::BackgroundRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

      default:
        return QVariant();
    }
  }
  // Progress
  else if (index.column() == Columns::Progress)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        if (!testUnavailableReason.empty())
        {
          return QString("Test not available: %1").arg(testUnavailableReason.c_str());
        }
        else if (bTestEnabled && bParentEnabled)
        {
          if (bIsSubTest)
          {
            if (TestResult.m_bExecuted)
            {
              return (TestResult.m_bSuccess) ? QString("Passed") : QString("Failed");
            }
            else
            {
              return QString("Pending");
            }
          }
          else
          {
            // Count sub-test status

            const plUInt32 iEnabled = m_pTestFramework->GetSubTestEnabledCount(pEntry->GetTestIndex());
            const plUInt32 iExecuted = m_pResult->GetSubTestCount(pEntry->GetTestIndex(), plTestResultQuery::Executed);
            const plUInt32 iSucceeded = m_pResult->GetSubTestCount(pEntry->GetTestIndex(), plTestResultQuery::Success);

            if (TestResult.m_bExecuted && iExecuted == iEnabled)
            {
              return (TestResult.m_bSuccess && iExecuted == iSucceeded) ? QString("Passed") : QString("Failed");
            }
            else
            {
              return QString("%1 / %2 Executed").arg(iExecuted).arg(iEnabled);
            }
          }
        }
        else
        {
          return QString("Disabled");
        }
      }
      case Qt::BackgroundRole:
      {
        QPalette palette = QApplication::palette();
        return palette.alternateBase().color();
      }
      case Qt::ForegroundRole:
      {
        if (!testUnavailableReason.empty())
        {
          QPalette palette = QApplication::palette();
          return palette.color(QPalette::Disabled, QPalette::Text);
        }
        else if (TestResult.m_bExecuted)
        {
          return TestResult.m_bSuccess ? m_SucessColor : m_FailedColor;
        }
        return QVariant();
      }
      case Qt::TextAlignmentRole:
      {
        return Qt::AlignRight;
      }

      default:
        return QVariant();
    }
  }

  return QVariant();
}

Qt::ItemFlags plQtTestModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  plQtTestModelEntry* pEntry = (plQtTestModelEntry*)index.internalPointer();
  if (pEntry == &m_Root)
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
}

QVariant plQtTestModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case Columns::Name:
        return QString("Name");
      case Columns::Status:
        return QString("Status");
      case Columns::Duration:
        return QString("Duration (ms)");
      case Columns::Errors:
        return QString("Errors / Output");
      case Columns::Asserts:
        return QString("Checks");
      case Columns::Progress:
        return QString("Progress");
    }
  }
  return QVariant();
}

QModelIndex plQtTestModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (!hasIndex(iRow, iColumn, parent))
    return QModelIndex();

  const plQtTestModelEntry* pParent = nullptr;

  if (!parent.isValid())
    pParent = &m_Root;
  else
    pParent = static_cast<plQtTestModelEntry*>(parent.internalPointer());

  plQtTestModelEntry* pEntry = pParent->GetSubEntry(iRow);
  return pEntry ? createIndex(iRow, iColumn, pEntry) : QModelIndex();
}

QModelIndex plQtTestModel::parent(const QModelIndex& index) const
{
  if (!index.isValid())
    return QModelIndex();

  plQtTestModelEntry* pChild = static_cast<plQtTestModelEntry*>(index.internalPointer());
  plQtTestModelEntry* pParent = pChild->GetParentEntry();

  if (pParent == &m_Root)
    return QModelIndex();

  return createIndex(pParent->GetIndexInParent(), 0, pParent);
}

int plQtTestModel::rowCount(const QModelIndex& parent) const
{
  if (parent.column() > 0)
    return 0;

  const plQtTestModelEntry* pParent = nullptr;

  if (!parent.isValid())
    pParent = &m_Root;
  else
    pParent = static_cast<plQtTestModelEntry*>(parent.internalPointer());

  return pParent->GetNumSubEntries();
}

int plQtTestModel::columnCount(const QModelIndex& parent) const
{
  return Columns::ColumnCount;
}

bool plQtTestModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  plQtTestModelEntry* pEntry = static_cast<plQtTestModelEntry*>(index.internalPointer());
  if (pEntry == nullptr || index.column() != Columns::Name || iRole != Qt::CheckStateRole)
    return false;

  if (pEntry->GetNodeType() == plQtTestModelEntry::TestNode)
  {
    m_pTestFramework->SetTestEnabled(pEntry->GetTestIndex(), value.toBool());
    TestDataChanged(pEntry->GetIndexInParent(), -1);

    // if a test gets enabled in the UI, and all sub-tests are currently disabled,
    // enable all sub-tests as well
    // if some set of sub-tests is already enabled and some are disabled,
    // do not mess with the user's choice of enabled tests
    bool bEnableSubTests = value.toBool();
    for (plUInt32 subIdx = 0; subIdx < pEntry->GetNumSubEntries(); ++subIdx)
    {
      if (m_pTestFramework->IsSubTestEnabled(pEntry->GetTestIndex(), subIdx))
      {
        bEnableSubTests = false;
        break;
      }
    }

    if (bEnableSubTests)
    {
      for (plUInt32 subIdx = 0; subIdx < pEntry->GetNumSubEntries(); ++subIdx)
      {
        m_pTestFramework->SetSubTestEnabled(pEntry->GetTestIndex(), subIdx, true);
        TestDataChanged(pEntry->GetIndexInParent(), subIdx);
      }
    }
  }
  else
  {
    m_pTestFramework->SetSubTestEnabled(pEntry->GetTestIndex(), pEntry->GetSubTestIndex(), value.toBool());
    TestDataChanged(pEntry->GetParentEntry()->GetIndexInParent(), pEntry->GetIndexInParent());
  }

  return true;
}


////////////////////////////////////////////////////////////////////////
// plQtTestModel public slots
////////////////////////////////////////////////////////////////////////

void plQtTestModel::UpdateModel()
{
  m_Root.ClearEntries();
  if (m_pResult == nullptr)
    return;

  const plUInt32 uiTestCount = m_pResult->GetTestCount();
  for (plUInt32 uiTestIndex = 0; uiTestIndex < uiTestCount; ++uiTestIndex)
  {
    plQtTestModelEntry* pTestModelEntry = new plQtTestModelEntry(m_pResult, uiTestIndex);
    m_Root.AddSubEntry(pTestModelEntry);

    const plUInt32 uiSubTestCount = m_pResult->GetSubTestCount(uiTestIndex);
    for (plUInt32 uiSubTestIndex = 0; uiSubTestIndex < uiSubTestCount; ++uiSubTestIndex)
    {
      plQtTestModelEntry* pSubTestModelEntry = new plQtTestModelEntry(m_pResult, uiTestIndex, uiSubTestIndex);
      pTestModelEntry->AddSubEntry(pSubTestModelEntry);
    }
  }
  // reset();
}


#endif

PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestModel);
