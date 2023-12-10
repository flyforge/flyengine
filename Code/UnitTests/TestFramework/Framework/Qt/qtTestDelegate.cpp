#include <TestFramework/TestFrameworkPCH.h>

#ifdef PLASMA_USE_QT

#  include <QApplication>
#  include <QPainter>
#  include <TestFramework/Framework/Qt/qtTestDelegate.h>
#  include <TestFramework/Framework/Qt/qtTestModel.h>

////////////////////////////////////////////////////////////////////////
// plQtTestDelegate public functions
////////////////////////////////////////////////////////////////////////

plQtTestDelegate::plQtTestDelegate(QObject* pParent)
  : QStyledItemDelegate(pParent)
{
}

plQtTestDelegate::~plQtTestDelegate() = default;

void plQtTestDelegate::paint(QPainter* pPainter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
  if (index.column() == plQtTestModel::Columns::Duration)
  {
    // We need to draw the alternate background color here because setting it via the model would
    // overwrite our duration bar.
    pPainter->save();
    pPainter->setPen(Qt::NoPen);
    pPainter->setBrush(option.palette.alternateBase());
    pPainter->drawRect(option.rect);
    pPainter->restore();

    bool bSuccess = false;
    float fProgress = index.data(plQtTestModel::UserRoles::Duration).toFloat(&bSuccess);

    // If we got a valid float from the model we can draw a small duration bar on top of the background.
    if (bSuccess)
    {
      QColor DurationColor = index.data(plQtTestModel::UserRoles::DurationColor).value<QColor>();
      QStyleOptionViewItem option2 = option;
      option2.palette.setBrush(QPalette::Base, QBrush(DurationColor));
      option2.rect.setWidth((int)((float)option2.rect.width() * fProgress));
      QApplication::style()->drawControl(QStyle::CE_ProgressBarGroove, &option2, pPainter);
    }
  }

  QStyledItemDelegate::paint(pPainter, option, index);
}

#endif

PLASMA_STATICLINK_FILE(TestFramework, TestFramework_Framework_Qt_qtTestDelegate);
