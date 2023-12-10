#pragma once

#include <EditorFramework/ui_SettingsTab.h>
#include <Foundation/Configuration/Plugin.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class plQtSettingsTab : public plQtDocumentWindow, Ui_SettingsTab
{
  Q_OBJECT

  PLASMA_DECLARE_SINGLETON(plQtSettingsTab);

public:
  plQtSettingsTab();
  ~plQtSettingsTab();

  virtual plString GetWindowIcon() const override;
  virtual plString GetDisplayNameShort() const override;

  virtual const char* GetWindowLayoutGroupName() const override { return "Settings"; }

protected Q_SLOTS:
  void on_OpenScene_clicked();
  void on_OpenProject_clicked();
  void on_GettingStarted_clicked();

private:
  virtual bool InternalCanCloseWindow() override;
  virtual void InternalCloseDocumentWindow() override;

  void ToolsProjectEventHandler(const plToolsProjectEvent& e);
};
