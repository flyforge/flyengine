#pragma once

#ifdef PLASMA_USE_QT

#  include <QAbstractItemModel>
#  include <QColor>
#  include <QIcon>
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

class plQtTestFramework;

/// \brief Helper class that stores the test hierarchy used in plQtTestModel.
class plQtTestModelEntry
{
public:
  plQtTestModelEntry(const plTestFrameworkResult* pResult, plInt32 iTestIndex = -1, plInt32 iSubTestIndex = -1);
  ~plQtTestModelEntry();

private:
  plQtTestModelEntry(plQtTestModelEntry&);
  void operator=(plQtTestModelEntry&);

public:
  enum plTestModelEntryType
  {
    RootNode,
    TestNode,
    SubTestNode
  };

  void ClearEntries();
  plUInt32 GetNumSubEntries() const;
  plQtTestModelEntry* GetSubEntry(plUInt32 uiIndex) const;
  void AddSubEntry(plQtTestModelEntry* pEntry);
  plQtTestModelEntry* GetParentEntry() const { return m_pParentEntry; }
  plUInt32 GetIndexInParent() const { return m_uiIndexInParent; }
  plTestModelEntryType GetNodeType() const;
  const plTestResultData* GetTestResult() const;
  plInt32 GetTestIndex() const { return m_iTestIndex; }
  plInt32 GetSubTestIndex() const { return m_iSubTestIndex; }

private:
  const plTestFrameworkResult* m_pResult;
  plInt32 m_iTestIndex;
  plInt32 m_iSubTestIndex;

  plQtTestModelEntry* m_pParentEntry = nullptr;
  plUInt32 m_uiIndexInParent = 0;
  std::deque<plQtTestModelEntry*> m_SubEntries;
};

/// \brief A Model that lists all unit tests and sub-tests in a tree.
class PLASMA_TEST_DLL plQtTestModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  plQtTestModel(QObject* pParent, plQtTestFramework* pTestFramework);
  virtual ~plQtTestModel();

  void Reset();
  void InvalidateAll();
  void TestDataChanged(plInt32 iTestIndex, plInt32 iSubTestIndex);

  struct UserRoles
  {
    enum Enum
    {
      Duration = Qt::UserRole,
      DurationColor = Qt::UserRole + 1,
    };
  };

  struct Columns
  {
    enum Enum
    {
      Name = 0,
      Status,
      Duration,
      Errors,
      Asserts,
      Progress,
      ColumnCount,
    };
  };

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;

public Q_SLOTS:
  void UpdateModel();

private:
  plQtTestFramework* m_pTestFramework;
  plTestFrameworkResult* m_pResult;
  plQtTestModelEntry m_Root;
  QColor m_SucessColor;
  QColor m_FailedColor;
  QColor m_TestColor;
  QColor m_SubTestColor;
  QIcon m_TestIcon;
  QIcon m_TestIconOff;
};

#endif

