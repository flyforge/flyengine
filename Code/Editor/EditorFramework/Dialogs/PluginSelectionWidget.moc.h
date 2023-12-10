#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/ui_PluginSelectionWidget.h>
#include <Foundation/Strings/String.h>
#include <QWidget>

struct plPluginBundleSet;
struct plPluginBundle;

class PLASMA_EDITORFRAMEWORK_DLL plQtPluginSelectionWidget : public QWidget, public Ui_PluginSelectionWidget
{
public:
  Q_OBJECT

public:
  plQtPluginSelectionWidget(QWidget* pParent);
  ~plQtPluginSelectionWidget();

  void SetPluginSet(plPluginBundleSet* pPluginSet);
  void SyncStateToSet();

private Q_SLOTS:
  void on_PluginsList_currentItemChanged(QListWidgetItem* current, QListWidgetItem* previous);
  void on_PluginsList_itemChanged(QListWidgetItem* item);
  void on_Template_currentIndexChanged(int index);

private:
  struct State
  {
    plString m_sID;
    plPluginBundle* m_pInfo = nullptr;
    bool m_bLoadCopy = false;
    bool m_bSelected = false;
    bool m_bIsDependency = false;
  };

  void UpdateInternalState();
  void ApplyRequired(plArrayPtr<plString> required);

  plHybridArray<State, 8> m_States;
  plPluginBundleSet* m_pPluginSet = nullptr;
};
