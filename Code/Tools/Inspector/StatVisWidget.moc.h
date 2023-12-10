#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <Inspector/ui_StatVisWidget.h>
#include <QAction>
#include <QGraphicsView>
#include <QListWidgetItem>
#include <ads/DockWidget.h>

class plQtStatVisWidget : public ads::CDockWidget, public Ui_StatVisWidget
{
public:
  Q_OBJECT

public:
  static const plUInt8 s_uiMaxColors = 9;

  plQtStatVisWidget(QWidget* pParent, plInt32 iWindowNumber);
  ~plQtStatVisWidget();

  void UpdateStats();

  static plQtStatVisWidget* s_pWidget;

  void AddStat(const plString& sStatPath, bool bEnabled = true, bool bRaiseWindow = true);

  void Save();
  void Load();

private Q_SLOTS:

  void on_ComboTimeframe_currentIndexChanged(int index);
  void on_LineName_textChanged(const QString& text);
  void on_SpinMin_valueChanged(double val);
  void on_SpinMax_valueChanged(double val);
  void on_ToggleVisible();
  void on_ButtonRemove_clicked();
  void on_ListStats_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);

public:
  QAction m_ShowWindowAction;

private:
  QGraphicsPathItem* m_pPath[s_uiMaxColors];
  QGraphicsPathItem* m_pPathMax;
  QGraphicsScene m_Scene;

  static plInt32 s_iCurColor;

  plTime m_DisplayInterval;

  plInt32 m_iWindowNumber;

  struct StatsData
  {
    QListWidgetItem* m_pListItem = nullptr;
    plUInt8 m_uiColor = 0;
  };

  plMap<plString, StatsData> m_Stats;
};

