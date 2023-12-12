#include <EditorFramework/Assets/TreeView.h>

plQtAssetTreeWidget::plQtAssetTreeWidget(QWidget* parent)
  : QTreeWidget(parent)
{
  setDragEnabled(true);
  setAcceptDrops(true);
}

void plQtAssetTreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
  if (event->source())
    event->acceptProposedAction();
}

void plQtAssetTreeWidget::dragMoveEvent(QDragMoveEvent* event)
{
  event->acceptProposedAction();
}

void plQtAssetTreeWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
  event->accept();
}

void plQtAssetTreeWidget::dropEvent(QDropEvent* event)
{
  if (!event->mimeData()->hasUrls())
    return;

  QList<QUrl> paths = event->mimeData()->urls();
  plString targetPar = indexAt(event->pos()).data(plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
  if (targetPar.IsEmpty())
  {
    return;
  }

  for (auto it = paths.begin(); it != paths.end(); it++)
  {
    plStringBuilder src = it->path().toUtf8().data();
    if (src.StartsWith("/"))
      src.Shrink(1, 0); // remove prepending '/' in name that appears for some reason

    plStringBuilder target = targetPar;
    target.AppendFormat("/{}", it->fileName().toUtf8());

    if (targetPar == src)
    {
      plLog::Error("Can't drop a file or folder on itself");
      continue;
    }


    if (!plOSFile::ExistsDirectory(src) && !plOSFile::ExistsFile(src))
    {
      plLog::Error("Cannot find file/folder to move : {}", src);
      return;
    }
    if (plOSFile::ExistsDirectory(src))
    {
      if (plOSFile::ExistsDirectory(target)) // ask to overwrite if target already exists
      {
        plStringBuilder msg = plStringBuilder();
        msg.Format("destination {} already exists", target);
        QMessageBox msgBox;
        msgBox.setText(msg.GetData());
        msgBox.setInformativeText("Overwrite existing folder?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (!msgBox.exec())
          return;

        if (plOSFile::DeleteFolder(target).Failed())
        {
          plLog::Error("Failed to delete folder {}", target);
          return;
        }
      }

      if (plOSFile::CreateDirectoryStructure(target).Succeeded())
      {
        if (plOSFile::CopyFolder(src, target).Failed())
        {
          plLog::Error("Failed to copy folder {} content", src);
        }
        if (plOSFile::DeleteFolder(src).Failed())
        {
          plLog::Error("Failed to delete folder {}", src);
        }
      }
      else
      {
        plLog::Error("Failed to copy folder {} to {}", src, target);
      }
    }
    else if (plOSFile::ExistsFile(src))
    {
      if (plOSFile::ExistsFile(target)) // ask to overwrite if target already exists
      {
        plStringBuilder msg = plStringBuilder();
        msg.Format("destination {} already exists", target);
        QMessageBox msgBox;
        msgBox.setText(msg.GetData());
        msgBox.setInformativeText("Overwrite existing file?");
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        if (!msgBox.exec())
          continue;

        if (plOSFile::DeleteFile(target).Failed())
        {
          plLog::Error("Failed to delete file {}", target);
          return;
        }
      }
      if (plOSFile::MoveFileOrDirectory(src, target).Failed())
      {
        plLog::Error("failed to move file or dir from {} to {}", src, target);
        return;
      }
    }
  }

  plAssetCurator::GetSingleton()->CheckFileSystem();
}

void plQtAssetTreeWidget::mousePressEvent(QMouseEvent* event)
{
  //override to clear the selection when clicking on empty space
  if (!indexAt(event->pos()).isValid())
  {
    selectionModel()->clear();
  }
  QTreeWidget::mousePressEvent(event);
}

QMimeData* plQtAssetTreeWidget::mimeData(const QList<QTreeWidgetItem*>& items) const
{
  QMimeData* mime = new QMimeData();
  QList<QUrl> urls = QList<QUrl>();
  
  for (int idx = 0; idx < items.size(); idx++)
  {
    QTreeWidgetItem* item = items[idx];
    plStringBuilder str = item->data(0, plQtAssetBrowserModel::UserRoles::AbsolutePath).toString().toUtf8().data();
    str.Prepend("/"); //needed otherwise QUrl deletes the root of the path
    QUrl url = QUrl(str.GetData(), QUrl::StrictMode);
    urls.append(url);
  }

  mime->setUrls(urls);
  return mime;
}

void plQtAssetTreeWidget::resetTree()
{
}
