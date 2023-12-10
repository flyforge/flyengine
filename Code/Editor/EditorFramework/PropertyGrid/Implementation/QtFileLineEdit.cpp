#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/PropertyGrid/QtFileLineEdit.moc.h>
#include <EditorFramework/PropertyGrid/FileBrowserPropertyWidget.moc.h>

plQtFileLineEdit::plQtFileLineEdit(plQtFilePropertyWidget* pParent)
  : QLineEdit(pParent)
{
  m_pOwner = pParent;
}

void plQtFileLineEdit::dragMoveEvent(QDragMoveEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidFileReference(qtToPlasmaString(str)))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragMoveEvent(e);
}

void plQtFileLineEdit::dragEnterEvent(QDragEnterEvent* e)
{
  if (e->mimeData()->hasUrls() && !e->mimeData()->urls().isEmpty())
  {
    QString str = e->mimeData()->urls()[0].toLocalFile();

    if (m_pOwner->IsValidFileReference(qtToPlasmaString(str)))
      e->acceptProposedAction();

    return;
  }

  QLineEdit::dragEnterEvent(e);
}

void plQtFileLineEdit::dropEvent(QDropEvent* e)
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
      setText(plMakeQString(sPath));
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
