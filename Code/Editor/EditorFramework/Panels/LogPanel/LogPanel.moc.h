#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/ui_LogPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class plQtLogModel;
struct plLoggingEventData;

/// \brief The application wide panel that shows the engine log output and the editor log output
class PL_EDITORFRAMEWORK_DLL plQtLogPanel : public plQtApplicationPanel, public Ui_LogPanel
{
  Q_OBJECT

  PL_DECLARE_SINGLETON(plQtLogPanel);

public:
  plQtLogPanel();
  ~plQtLogPanel();

protected:
  virtual void ToolsProjectEventHandler(const plToolsProjectEvent& e) override;

private Q_SLOTS:
  void OnNewWarningsOrErrors(const char* szText, bool bError);

private:
  void LogWriter(const plLoggingEventData& e);
  void EngineProcessMsgHandler(const plEditorEngineProcessConnection::Event& e);
  void UiServiceEventHandler(const plQtUiServices::Event& e);

  plUInt32 m_uiIgnoredNumErrors = 0;
  plUInt32 m_uiIgnoreNumWarnings = 0;
  plUInt32 m_uiKnownNumErrors = 0;
  plUInt32 m_uiKnownNumWarnings = 0;
};

