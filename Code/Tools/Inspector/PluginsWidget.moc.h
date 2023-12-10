#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Inspector/ui_PluginsWidget.h>
#include <ads/DockWidget.h>

class plQtPluginsWidget : public ads::CDockWidget, public Ui_PluginsWidget
{
public:
  Q_OBJECT

public:
  plQtPluginsWidget(QWidget* pParent = 0);

  static plQtPluginsWidget* s_pWidget;

public:
  static void ProcessTelemetry(void* pUnuseed);

  void ResetStats();
  void UpdateStats();

private:
  void UpdatePlugins();

  struct PluginsData
  {
    bool m_bReloadable;
    plString m_sDependencies;
  };

  bool m_bUpdatePlugins;
  plMap<plString, PluginsData> m_Plugins;
};

