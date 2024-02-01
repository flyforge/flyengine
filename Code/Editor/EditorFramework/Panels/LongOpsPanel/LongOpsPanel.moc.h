#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_LongOpsPanel.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DockPanels/ApplicationPanel.moc.h>

#include <QTimer>

struct plLongOpControllerEvent;

/// \brief This panel listens to events from plLongOpControllerManager and displays all currently known long operations
class PL_EDITORFRAMEWORK_DLL plQtLongOpsPanel : public plQtApplicationPanel, public Ui_LongOpsPanel
{
  Q_OBJECT

  PL_DECLARE_SINGLETON(plQtLongOpsPanel);

public:
  plQtLongOpsPanel();
  ~plQtLongOpsPanel();

private:
  void LongOpsEventHandler(const plLongOpControllerEvent& e);
  void RebuildTable();
  void UpdateTable();

  bool m_bUpdateTimerRunning = false;
  bool m_bRebuildTable = true;
  bool m_bUpdateTable = false;
  plHashTable<plUuid, plUInt32> m_LongOpGuidToRow;


private Q_SLOTS:
  void StartUpdateTimer();
  void UpdateUI();
  void OnClickButton(bool);
  void OnCellDoubleClicked(int row, int column);
};

