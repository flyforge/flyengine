#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/LogEntry.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QAbstractItemModel>

/// \brief The Qt model that represents log output for a view
class PLASMA_GUIFOUNDATION_DLL plQtLogModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  plQtLogModel(QObject* parent);
  void Clear();
  void SetLogLevel(plLogMsgType::Enum LogLevel);
  void SetSearchText(const char* szText);
  void AddLogMsg(const plLogEntry& msg);

  plUInt32 GetVisibleItemCount() const { return m_VisibleMessages.GetCount(); }

  plUInt32 GetNumErrors() const { return m_uiNumErrors; }
  plUInt32 GetNumSeriousWarnings() const { return m_uiNumSeriousWarnings; }
  plUInt32 GetNumWarnings() const { return m_uiNumWarnings; }

public: // QAbstractItemModel interface
  virtual QVariant data(const QModelIndex& index, int role) const override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

Q_SIGNALS:
  void NewErrorsOrWarnings(const char* szLatest, bool bError);

private Q_SLOTS:
  /// \brief Adds queued messages from a different thread to the model.
  void ProcessNewMessages();

private:
  void Invalidate();
  bool IsFiltered(const plLogEntry& lm) const;
  void UpdateVisibleEntries() const;

  plLogMsgType::Enum m_LogLevel;
  plString m_sSearchText;
  plDeque<plLogEntry> m_AllMessages;

  mutable bool m_bIsValid;
  mutable plDeque<const plLogEntry*> m_VisibleMessages;
  mutable plHybridArray<const plLogEntry*, 16> m_BlockQueue;

  mutable plMutex m_NewMessagesMutex;
  plDeque<plLogEntry> m_NewMessages;

  plUInt32 m_uiNumErrors = 0;
  plUInt32 m_uiNumSeriousWarnings = 0;
  plUInt32 m_uiNumWarnings = 0;
};

