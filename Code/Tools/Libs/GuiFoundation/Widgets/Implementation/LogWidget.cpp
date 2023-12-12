#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>
#include <QClipboard>
#include <QKeyEvent>

plQtLogWidget::plQtLogWidget(QWidget* parent)
  : QWidget(parent)
{
  setupUi(this);

  m_pLog = new plQtLogModel(this);
  ListViewLog->setModel(m_pLog);
  ListViewLog->setUniformItemSizes(true);
  ListViewLog->installEventFilter(this);
  connect(m_pLog, &QAbstractItemModel::rowsInserted, this, [this](const QModelIndex& parent, int first, int last) { ScrollToBottomIfAtEnd(first); });

  const int logIndex = ((int)plLogMsgType::All - (int)plLogMsgType::InfoMsg);
  ComboFilter->setCurrentIndex(logIndex);
}

plQtLogWidget::~plQtLogWidget() = default;

void plQtLogWidget::ShowControls(bool show)
{
  ButtonClearLog->setVisible(show);
  ComboFilter->setVisible(show);
  Search->setVisible(show);
}

plQtLogModel* plQtLogWidget::GetLog()
{
  return m_pLog;
}

plQtSearchWidget* plQtLogWidget::GetSearchWidget()
{
  return Search;
}

void plQtLogWidget::SetLogLevel(plLogMsgType::Enum logLevel)
{
  PLASMA_ASSERT_DEBUG(logLevel >= (int)plLogMsgType::ErrorMsg && logLevel <= plLogMsgType::All, "Invalid log level set.");
  ComboFilter->setCurrentIndex((int)plLogMsgType::All - (int)logLevel);
}

plLogMsgType::Enum plQtLogWidget::GetLogLevel() const
{
  int index = ComboFilter->currentIndex();
  return (plLogMsgType::Enum)((int)plLogMsgType::All - index);
}

bool plQtLogWidget::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject == ListViewLog)
  {
    if (pEvent->type() == QEvent::ShortcutOverride)
    {
      // Intercept copy
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
      if (keyEvent->matches(QKeySequence::StandardKey::Copy))
      {
        keyEvent->accept();
        return true;
      }
    }
    else if (pEvent->type() == QEvent::KeyPress)
    {
      // Copy entire selection
      QKeyEvent* keyEvent = static_cast<QKeyEvent*>(pEvent);
      if (keyEvent->matches(QKeySequence::StandardKey::Copy))
      {
        QModelIndexList selection = ListViewLog->selectionModel()->selectedRows(0);
        QStringList sTemp;
        sTemp.reserve(selection.count());
        for (const QModelIndex& index : selection)
        {
          QString sLine = m_pLog->data(index, Qt::DisplayRole).toString();
          sTemp.push_back(sLine);
        }

        QString sFullText = sTemp.join(QStringLiteral("\n"));
        QApplication::clipboard()->setText(sFullText);
        keyEvent->accept();
        return true;
      }
    }
  }

  return false;
}

void plQtLogWidget::ScrollToBottomIfAtEnd(int iNumElements)
{
  if (ListViewLog->selectionModel()->hasSelection())
  {
    if (ListViewLog->selectionModel()->selectedIndexes()[0].row() + 1 >= iNumElements)
    {
      ListViewLog->selectionModel()->clearSelection();
      ListViewLog->scrollToBottom();
    }
  }
  else
    ListViewLog->scrollToBottom();
}

void plQtLogWidget::on_ButtonClearLog_clicked()
{
  m_pLog->Clear();
}

void plQtLogWidget::on_Search_textChanged(const QString& text)
{
  m_pLog->SetSearchText(text.toUtf8().data());
}

void plQtLogWidget::on_ComboFilter_currentIndexChanged(int index)
{
  const plLogMsgType::Enum LogLevel = (plLogMsgType::Enum)((int)plLogMsgType::All - index);
  m_pLog->SetLogLevel(LogLevel);
}
