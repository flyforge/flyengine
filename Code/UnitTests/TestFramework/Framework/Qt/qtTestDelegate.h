#pragma once

#ifdef PLASMA_USE_QT

#  include <QStyledItemDelegate>
#  include <TestFramework/Framework/Qt/qtTestFramework.h>
#  include <TestFramework/TestFrameworkDLL.h>

class plQtTestFramework;

/// \brief Delegate for plQtTestModel which shows bars for the test durations.
class PLASMA_TEST_DLL plQtTestDelegate : public QStyledItemDelegate
{
  Q_OBJECT
public:
  plQtTestDelegate(QObject* pParent);
  virtual ~plQtTestDelegate();

public: // QStyledItemDelegate interface
  virtual void paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
};

#endif

