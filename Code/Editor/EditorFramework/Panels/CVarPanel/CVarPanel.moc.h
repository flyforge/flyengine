#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/Widgets/CVarWidget.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class plQtCVarWidget;

class PLASMA_EDITORFRAMEWORK_DLL plQtCVarPanel : public plQtApplicationPanel
{
  Q_OBJECT

  PLASMA_DECLARE_SINGLETON(plQtCVarPanel);

public:
  plQtCVarPanel();
  ~plQtCVarPanel();

protected:
  virtual void ToolsProjectEventHandler(const plToolsProjectEvent& e) override;

private Q_SLOTS:
  void UpdateUI();
  void BoolChanged(const char* szCVar, bool newValue);
  void FloatChanged(const char* szCVar, float newValue);
  void IntChanged(const char* szCVar, int newValue);
  void StringChanged(const char* szCVar, const char* newValue);

private:
  void EngineProcessMsgHandler(const PlasmaEditorEngineProcessConnection::Event& e);

  plQtCVarWidget* m_pCVarWidget = nullptr;

  plMap<plString, plCVarWidgetData> m_EngineCVarState;

  bool m_bUpdateUI = false;
  bool m_bRebuildUI = false;
  bool m_bUpdateConsole = false;
  plStringBuilder m_sCommandResult;
};

