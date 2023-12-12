#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <QWidget>

class plAssetDocument;
class plQtEngineDocumentWindow;
class QGridLayout;
class plQtViewWidgetContainer;
class plQtEngineViewWidget;
struct PlasmaEngineViewConfig;
struct PlasmaEngineViewPreferences;

class PLASMA_EDITORFRAMEWORK_DLL plQtQuadViewWidget : public QWidget
{
  Q_OBJECT
public:
  typedef plDelegate<plQtEngineViewWidget*(plQtEngineDocumentWindow*, PlasmaEngineViewConfig*)> ViewFactory;
  plQtQuadViewWidget(plAssetDocument* pDocument, plQtEngineDocumentWindow* pWindow, ViewFactory viewFactory, const char* szViewToolBarMapping);
  ~plQtQuadViewWidget();

  const plHybridArray<plQtViewWidgetContainer*, 4>& GetActiveMainViews() { return m_ActiveMainViews; }

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

protected:
  void SaveViewConfig(const PlasmaEngineViewConfig& cfg, PlasmaEngineViewPreferences& pref) const;
  void LoadViewConfig(PlasmaEngineViewConfig& cfg, PlasmaEngineViewPreferences& pref);
  void SaveViewConfigs() const;
  void LoadViewConfigs();
  void CreateViews(bool bQuad);

private:
  plAssetDocument* m_pDocument;
  plQtEngineDocumentWindow* m_pWindow;
  ViewFactory m_ViewFactory;
  plString m_sViewToolBarMapping;

  PlasmaEngineViewConfig m_ViewConfigSingle;
  PlasmaEngineViewConfig m_ViewConfigQuad[4];
  plHybridArray<plQtViewWidgetContainer*, 4> m_ActiveMainViews;
  QGridLayout* m_pViewLayout;
};

