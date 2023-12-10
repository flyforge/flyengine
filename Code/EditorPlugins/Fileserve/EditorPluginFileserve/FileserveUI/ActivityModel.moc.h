#pragma once

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>
#include <Foundation/Containers/Deque.h>
#include <QAbstractListModel>

enum class plFileserveActivityType
{
  StartServer,
  StopServer,
  ClientConnect,
  ClientReconnected,
  ClientDisconnect,
  Mount,
  MountFailed,
  Unmount,
  ReadFile,
  WriteFile,
  DeleteFile,
  Other
};

struct plQtFileserveActivityItem
{
  QString m_Text;
  plFileserveActivityType m_Type;
};

class PLASMA_EDITORPLUGINFILESERVE_DLL plQtFileserveActivityModel : public QAbstractListModel
{
  Q_OBJECT

public:
  plQtFileserveActivityModel(QWidget* parent);

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  plQtFileserveActivityItem& AppendItem();
  void UpdateView();

  void Clear();
private Q_SLOTS:
  void UpdateViewSlot();

private:
  bool m_bTimerRunning = false;
  plUInt32 m_uiAddedItems = 0;
  plDeque<plQtFileserveActivityItem> m_Items;
};

