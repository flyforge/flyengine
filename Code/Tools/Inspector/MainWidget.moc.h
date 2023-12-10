#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <Inspector/ui_MainWidget.h>
#include <QMainWindow>
#include <ads/DockManager.h>

class QTreeWidgetItem;

class plQtMainWidget : public ads::CDockWidget, public Ui_MainWidget
{
  Q_OBJECT
public:
  static plQtMainWidget* s_pWidget;

  plQtMainWidget(QWidget* pParent = nullptr);
  ~plQtMainWidget();

  void ResetStats();
  void UpdateStats();
  virtual void closeEvent(QCloseEvent* pEvent) override;

  static void ProcessTelemetry(void* pUnuseed);

public Q_SLOTS:
  void ShowStatIn(bool);

private Q_SLOTS:
  void on_ButtonConnect_clicked();

  void on_TreeStats_itemChanged(QTreeWidgetItem* item, int column);
  void on_TreeStats_customContextMenuRequested(const QPoint& p);

private:
  void SaveFavorites();
  void LoadFavorites();

  QTreeWidgetItem* CreateStat(plStringView sPath, bool bParent);
  void SetFavorite(const plString& sStat, bool bFavorite);

  plUInt32 m_uiMaxStatSamples;
  plTime m_MaxGlobalTime;

  struct StatSample
  {
    plTime m_AtGlobalTime;
    double m_Value;
  };

  struct StatData
  {
    plDeque<StatSample> m_History;

    plVariant m_Value;
    QTreeWidgetItem* m_pItem;
    QTreeWidgetItem* m_pItemFavorite;

    StatData()
    {
      m_pItem = nullptr;
      m_pItemFavorite = nullptr;
    }
  };

  friend class plQtStatVisWidget;
  plMap<plString, StatData> m_Stats;
  plSet<plString> m_Favorites;
};

