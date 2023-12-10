#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <Inspector/ui_CVarsWidget.h>
#include <ads/DockWidget.h>

class plQtCVarsWidget : public ads::CDockWidget, public Ui_CVarsWidget
{
public:
  Q_OBJECT

public:
  plQtCVarsWidget(QWidget* pParent = 0);

  static plQtCVarsWidget* s_pWidget;

private Q_SLOTS:
  void BoolChanged(plStringView sCVar, bool newValue);
  void FloatChanged(plStringView sCVar, float newValue);
  void IntChanged(plStringView sCVar, int newValue);
  void StringChanged(plStringView sCVar, plStringView sNewValue);

public:
  static void ProcessTelemetry(void* pUnuseed);
  static void ProcessTelemetryConsole(void* pUnuseed);

  void ResetStats();

private:
  // void UpdateCVarsTable(bool bRecreate);


  void SendCVarUpdateToServer(plStringView sName, const plCVarWidgetData& cvd);
  void SyncAllCVarsToServer();

  plMap<plString, plCVarWidgetData> m_CVars;
  plMap<plString, plCVarWidgetData> m_CVarsBackup;
};

