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
struct plEngineViewConfig;
struct plEngineViewPreferences;

class PL_EDITORFRAMEWORK_DLL plQtQuadViewWidget : public QWidget
{
  Q_OBJECT
public:
  using ViewFactory = plDelegate<plQtEngineViewWidget*(plQtEngineDocumentWindow*, plEngineViewConfig*)>;
  plQtQuadViewWidget(plAssetDocument* pDocument, plQtEngineDocumentWindow* pWindow, ViewFactory viewFactory, const char* szViewToolBarMapping);
  ~plQtQuadViewWidget();

  const plHybridArray<plQtViewWidgetContainer*, 4>& GetActiveMainViews() { return m_ActiveMainViews; }

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

protected:
  void SaveViewConfig(const plEngineViewConfig& cfg, plEngineViewPreferences& pref) const;
  void LoadViewConfig(plEngineViewConfig& cfg, plEngineViewPreferences& pref);
  void SaveViewConfigs() const;
  void LoadViewConfigs();
  void CreateViews(bool bQuad);

private:
  plAssetDocument* m_pDocument;
  plQtEngineDocumentWindow* m_pWindow;
  ViewFactory m_ViewFactory;
  plString m_sViewToolBarMapping;

  plEngineViewConfig m_ViewConfigSingle;
  plEngineViewConfig m_ViewConfigQuad[4];
  plHybridArray<plQtViewWidgetContainer*, 4> m_ActiveMainViews;
  QGridLayout* m_pViewLayout;
};

