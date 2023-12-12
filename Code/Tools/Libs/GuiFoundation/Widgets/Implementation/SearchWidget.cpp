#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QKeyEvent>
#include <QLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>

plQtSearchWidget::plQtSearchWidget(QWidget* pParent)
{
  setLayout(new QHBoxLayout(this));
  setContentsMargins(0, 0, 0, 0);
  layout()->setContentsMargins(0, 0, 0, 0);
  layout()->setSpacing(0);

  {
    m_pClearButton = new QPushButton(this);
    m_pClearButton->setAutoDefault(false);
    m_pClearButton->setDefault(false);
    m_pClearButton->setEnabled(false);
    m_pClearButton->setIcon(QIcon(":/GuiFoundation/Icons/Delete.svg"));
  }

  {
    m_pLineEdit = new QLineEdit(this);
    m_pLineEdit->setPlaceholderText("Search");
    m_pLineEdit->installEventFilter(this);

    setFocusProxy(m_pLineEdit);
  }

  QSpacerItem* spacer1 = new QSpacerItem(8, 24, QSizePolicy::Minimum, QSizePolicy::Minimum);
  QSpacerItem* spacer2 = new QSpacerItem(8, 24, QSizePolicy::Minimum, QSizePolicy::Minimum);
  QSpacerItem* spacer3 = new QSpacerItem(8, 24, QSizePolicy::Minimum, QSizePolicy::Minimum);

  layout()->addItem(spacer1);
  layout()->addWidget(m_pLineEdit);
  layout()->addItem(spacer2);
  layout()->addWidget(m_pClearButton);
  layout()->addItem(spacer3);

  connect(m_pLineEdit, &QLineEdit::textChanged, this, &plQtSearchWidget::onLineEditTextChanged);
  connect(m_pClearButton, &QPushButton::clicked, this, &plQtSearchWidget::onClearButtonClicked);
}

void plQtSearchWidget::setText(const QString& text)
{
  m_pLineEdit->setText(text);
}

QString plQtSearchWidget::text() const
{
  return m_pLineEdit->text();
}

void plQtSearchWidget::setPlaceholderText(const QString& text)
{
  m_pLineEdit->setPlaceholderText(text);
}

void plQtSearchWidget::onLineEditTextChanged(const QString& text)
{
  m_pClearButton->setEnabled(!text.isEmpty());

  Q_EMIT textChanged(text);
}

void plQtSearchWidget::onClearButtonClicked(bool checked)
{
  m_pLineEdit->setText(QString());
  m_pLineEdit->setFocus();
}

bool plQtSearchWidget::eventFilter(QObject* obj, QEvent* e)
{
  if (obj == m_pLineEdit)
  {
    if (e->type() == QEvent::KeyPress)
    {
      QKeyEvent* pEvent = static_cast<QKeyEvent*>(e);

      if (pEvent->key() == Qt::Key_Escape && !text().isEmpty())
      {
        setText("");
        return true;
      }

      if (pEvent->key() == Qt::Key_Return || pEvent->key() == Qt::Key_Enter)
      {
        Q_EMIT enterPressed();
        return true;
      }

      if (pEvent->key() == Qt::Key_Up || pEvent->key() == Qt::Key_Down || pEvent->key() == Qt::Key_Tab || pEvent->key() == Qt::Key_Backtab || pEvent->key() == Qt::Key_F1 || pEvent->key() == Qt::Key_F2 || pEvent->key() == Qt::Key_F3)
      {
        Q_EMIT specialKeyPressed((Qt::Key)pEvent->key());
        return true;
      }
    }
  }

  return false;
}

void plQtSearchWidget::showEvent(QShowEvent* e)
{
  QWidget::showEvent(e);

  Q_EMIT visibleEvent();
}
