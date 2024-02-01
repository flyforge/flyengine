#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>

plQtAssetLineEdit::plQtAssetLineEdit(QWidget* pParent /*= nullptr*/)
  : QLineEdit(pParent)
{
}

void plQtAssetLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragMoveEvent(e);
}

void plQtAssetLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidAssetType(str.toUtf8().data()))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragEnterEvent(e);
}

void plQtAssetLineEdit::dropEvent(QDropEvent* e)
{
  if (e->source() == this)
  {
    QLineEdit::dropEvent(e);
    return;
  }

  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    plString sPath = str.toUtf8().data();
    if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    {
      setText(QString::fromUtf8(sPath.GetData()));
    }
    else
      setText(QString());

    return;
  }


  if (e->mimeData()->hasText())
  {
    QString str = e->mimeData()->text();

    plString sPath = str.toUtf8().data();
    if (plQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sPath))
    {
      setText(QString::fromUtf8(sPath.GetData()));
    }
    else
      setText(QString());

    return;
  }
}

void plQtAssetLineEdit::paintEvent(QPaintEvent* e)
{
  if (hasFocus())
  {
    QLineEdit::paintEvent(e);
  }
  else
  {
    QPainter p(this);

    // Paint background
    QStyleOptionFrame panel;
    initStyleOption(&panel);
    style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);

    // Clip to line edit contents
    QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);
    auto margins = textMargins();
    r = r.marginsRemoved(margins);
    p.setClipRect(r);

    // Render asset name
    plStringBuilder sText = qtToPlString(text());
    if (sText.IsEmpty())
    {
      sText = qtToPlString(placeholderText());
    }

    plStringView sFinalText = sText;

    if (m_pOwner->IsValidAssetType(sText))
    {
      if (const char* szPipe = sFinalText.FindLastSubString("|"))
      {
        sFinalText = plStringView(szPipe + 1);
      }
      else
      {
        sFinalText = sFinalText.GetFileName();
      }
    }

    r.adjust(2, 0, 2, 0);
    QTextOption opt(Qt::AlignLeft | Qt::AlignVCenter);
    opt.setWrapMode(QTextOption::NoWrap);
    p.drawText(r, plMakeQString(sFinalText), opt);
  }
}
