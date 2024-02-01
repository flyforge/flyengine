#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Core/Console/Console.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/ui_CVarWidget.h>
#include <QItemDelegate>
#include <QPointer>
#include <QWidget>

class QStandardItemModel;
class QSortFilterProxyModel;
class plQtCVarModel;
class plQtCVarWidget;

class plQtCVarItemDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  explicit plQtCVarItemDelegate(QObject* pParent = nullptr)
    : QItemDelegate(pParent)
  {
  }

  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void setEditorData(QWidget* pEditor, const QModelIndex& index) const override;
  virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;

  plQtCVarModel* m_pModel = nullptr;

private:
  mutable QModelIndex m_Index;

private Q_SLOTS:
  void onComboChanged(int);
};

class plQtCVarModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  plQtCVarModel(plQtCVarWidget* pOwner);
  ~plQtCVarModel();

  void BeginResetModel();
  void EndResetModel();

public: // QAbstractItemModel interface
  virtual QVariant headerData(int iSection, Qt::Orientation orientation, int iRole = Qt::DisplayRole) const override;
  virtual QVariant data(const QModelIndex& index, int iRole) const override;
  virtual bool setData(const QModelIndex& index, const QVariant& value, int iRole = Qt::EditRole) override;
  virtual Qt::ItemFlags flags(const QModelIndex& index) const override;
  virtual QModelIndex index(int iRow, int iColumn, const QModelIndex& parent = QModelIndex()) const override;
  virtual QModelIndex parent(const QModelIndex& index) const override;
  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override;
  virtual int columnCount(const QModelIndex& parent = QModelIndex()) const override;

public:
  struct Entry
  {
    plString m_sFullName;
    QString m_sDisplayString;
    Entry* m_pParentEntry = nullptr;
    plDynamicArray<Entry*> m_ChildEntries;

    QString m_sPlugin;      // in which plugin a CVar is defined
    QString m_sDescription; // CVar description text
    plVariant m_Value;
  };

  Entry* CreateEntry(const char* szName);

  plQtCVarWidget* m_pOwner = nullptr;
  plDynamicArray<Entry*> m_RootEntries;
  plDeque<Entry> m_AllEntries;
};

/// \brief Data used by plQtCVarWidget to represent CVar states
struct PL_GUIFOUNDATION_DLL plCVarWidgetData
{
  mutable bool m_bNewEntry = true;

  plString m_sPlugin;      // in which plugin a CVar is defined
  plString m_sDescription; // CVar description text
  plUInt8 m_uiType = 0;    // plCVarType

  // 'union' over the different possible CVar types
  bool m_bValue = false;
  float m_fValue = 0.0f;
  plInt32 m_iValue = 0;
  plString m_sValue;
};

/// \brief Displays CVar values in a table and allows to modify them.
class PL_GUIFOUNDATION_DLL plQtCVarWidget : public QWidget, public Ui_CVarWidget
{
  Q_OBJECT

public:
  plQtCVarWidget(QWidget* pParent);
  ~plQtCVarWidget();

  /// \brief Clears the table
  void Clear();

  /// \brief Recreates the full UI. This is necessary when elements were added or removed.
  void RebuildCVarUI(const plMap<plString, plCVarWidgetData>& cvars);

  /// \brief Updates the existing UI. This is sufficient if values changed only.
  void UpdateCVarUI(const plMap<plString, plCVarWidgetData>& cvars);

  void AddConsoleStrings(const plStringBuilder& sEncoded);

  plConsole& GetConsole() { return m_Console; }

Q_SIGNALS:
  void onBoolChanged(const char* szCVar, bool bNewValue);
  void onFloatChanged(const char* szCVar, float fNewValue);
  void onIntChanged(const char* szCVar, int iNewValue);
  void onStringChanged(const char* szCVar, const char* szNewValue);

private Q_SLOTS:
  void SearchTextChanged(const QString& text);
  void ConsoleEnterPressed();
  void ConsoleSpecialKeyPressed(Qt::Key key);

private:
  QPointer<plQtCVarModel> m_pItemModel;
  QPointer<QSortFilterProxyModel> m_pFilterModel;
  QPointer<plQtCVarItemDelegate> m_pItemDelegate;

  void OnConsoleEvent(const plConsoleEvent& e);

  plConsole m_Console;
};
