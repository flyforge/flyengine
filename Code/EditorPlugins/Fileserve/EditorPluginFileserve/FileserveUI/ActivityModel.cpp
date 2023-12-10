#include <EditorPluginFileserve/EditorPluginFileservePCH.h>

#include <EditorPluginFileserve/FileserveUI/ActivityModel.moc.h>

plQtFileserveActivityModel::plQtFileserveActivityModel(QWidget* parent)
  : QAbstractListModel(parent)
{
}

int plQtFileserveActivityModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return m_Items.GetCount() - m_uiAddedItems;
}

int plQtFileserveActivityModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant plQtFileserveActivityModel::data(const QModelIndex& index, int role /*= Qt::DisplayRole*/) const
{
  if (!index.isValid())
    return QVariant();

  const auto& item = m_Items[index.row()];

  if (role == Qt::ToolTipRole)
  {
    if (item.m_Type == plFileserveActivityType::ReadFile)
    {
      return QString("[TIME] == File was not transferred because the timestamps match on server and client.\n"
                     "[HASH] == File was not transferred because the file hashes matched on server and client.\n"
                     "[N/A] == File does not exist on the server (in the requested data directory).");
    }
  }

  if (index.column() == 0)
  {
    if (role == Qt::DisplayRole)
    {
      switch (item.m_Type)
      {
        case plFileserveActivityType::StartServer:
          return "Server Started";
        case plFileserveActivityType::StopServer:
          return "Server Stopped";
        case plFileserveActivityType::ClientConnect:
          return "Client Connected";
        case plFileserveActivityType::ClientReconnected:
          return "Client Re-connected";
        case plFileserveActivityType::ClientDisconnect:
          return "Client Disconnect";
        case plFileserveActivityType::Mount:
          return "Mount";
        case plFileserveActivityType::MountFailed:
          return "Failed Mount";
        case plFileserveActivityType::Unmount:
          return "Unmount";
        case plFileserveActivityType::ReadFile:
          return "Read";
        case plFileserveActivityType::WriteFile:
          return "Write";
        case plFileserveActivityType::DeleteFile:
          return "Delete";

        default:
          return QVariant();
      }
    }

    if (role == Qt::ForegroundRole)
    {
      switch (item.m_Type)
      {
        case plFileserveActivityType::StartServer:
          return QColor::fromRgb(0, 200, 0);
        case plFileserveActivityType::StopServer:
          return QColor::fromRgb(200, 200, 0);

        case plFileserveActivityType::ClientConnect:
        case plFileserveActivityType::ClientReconnected:
          return QColor::fromRgb(50, 200, 0);
        case plFileserveActivityType::ClientDisconnect:
          return QColor::fromRgb(250, 100, 0);

        case plFileserveActivityType::Mount:
          return QColor::fromRgb(0, 0, 200);
        case plFileserveActivityType::MountFailed:
          return QColor::fromRgb(255, 0, 0);
        case plFileserveActivityType::Unmount:
          return QColor::fromRgb(150, 0, 200);

        case plFileserveActivityType::ReadFile:
          return QColor::fromRgb(100, 100, 100);
        case plFileserveActivityType::WriteFile:
          return QColor::fromRgb(255, 150, 0);
        case plFileserveActivityType::DeleteFile:
          return QColor::fromRgb(200, 50, 50);

        default:
          return QVariant();
      }
    }
  }

  if (index.column() == 1)
  {
    if (role == Qt::DisplayRole)
    {
      return item.m_Text;
    }
  }

  return QVariant();
}


QVariant plQtFileserveActivityModel::headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const
{
  if (role == Qt::DisplayRole)
  {
    if (section == 0)
    {
      return "Type";
    }

    if (section == 1)
    {
      return "Action";
    }
  }

  return QVariant();
}

plQtFileserveActivityItem& plQtFileserveActivityModel::AppendItem()
{
  if (!m_bTimerRunning)
  {
    m_bTimerRunning = true;

    QTimer::singleShot(250, this, &plQtFileserveActivityModel::UpdateViewSlot);
  }

  m_uiAddedItems++;
  return m_Items.ExpandAndGetRef();
}

void plQtFileserveActivityModel::UpdateView()
{
  if (m_uiAddedItems == 0)
    return;

  beginInsertRows(QModelIndex(), m_Items.GetCount() - m_uiAddedItems, m_Items.GetCount() - 1);
  insertRows(m_Items.GetCount(), m_uiAddedItems, QModelIndex());
  m_uiAddedItems = 0;
  endInsertRows();
}

void plQtFileserveActivityModel::Clear()
{
  m_Items.Clear();
  m_uiAddedItems = 0;

  beginResetModel();
  endResetModel();
}

void plQtFileserveActivityModel::UpdateViewSlot()
{
  m_bTimerRunning = false;

  UpdateView();
}
