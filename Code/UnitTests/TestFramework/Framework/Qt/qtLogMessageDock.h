#pragma once

#ifdef PLASMA_USE_QT

#  include <QAbstractItemModel>
#  include <QDockWidget>
#  include <TestFramework/TestFrameworkDLL.h>
#  include <TestFramework/ui_qtLogMessageDock.h>
#  include <vector>

class plQtTestFramework;
struct plTestResultData;
class plQtLogMessageModel;
class plTestFrameworkResult;

/// \brief Dock widget that lists the output of a given plResult struct.
class PLASMA_TEST_DLL plQtLogMessageDock : public QDockWidget, public Ui_qtLogMessageDock
{
  Q_OBJECT
public:
  plQtLogMessageDock(QObject* pParent, const plTestFrameworkResult* pResult);
  virtual ~plQtLogMessageDock();

public Q_SLOTS:
  void resetModel();
  void currentTestResultChanged(const plTestResultData* pTestResult);
  void currentTestSelectionChanged(const plTestResultData* pTestResult);

private:
  plQtLogMessageModel* m_pModel;
};

/// \brief Model used by plQtLogMessageDock to list the output entries in plResult.
class PLASMA_TEST_DLL plQtLogMessageModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  plQtLogMessageModel(QObject* pParent, const plTestFrameworkResult* pResult);
  virtual ~plQtLogMessageModel();

  void resetModel();
  QModelIndex GetFirstIndexOfTestSelection();
  QModelIndex GetLastIndexOfTestSelection();

public Q_SLOTS:
  void currentTestResultChanged(const plTestResultData* pTestResult);
  void currentTestSelectionChanged(const plTestResultData* pTestResult);

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
  void UpdateVisibleEntries();

private:
  const plTestResultData* m_pCurrentTestSelection;
  const plTestFrameworkResult* m_pTestResult;
  std::vector<plUInt32> m_VisibleEntries;
  std::vector<plUInt8> m_VisibleEntriesIndention;
};

#endif

