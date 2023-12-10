#include <Inspector/InspectorPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Inspector/CVarsWidget.moc.h>
#include <Inspector/DataTransferWidget.moc.h>
#include <Inspector/FileWidget.moc.h>
#include <Inspector/GlobalEventsWidget.moc.h>
#include <Inspector/InputWidget.moc.h>
#include <Inspector/LogDockWidget.moc.h>
#include <Inspector/MainWindow.moc.h>
#include <Inspector/MemoryWidget.moc.h>
#include <Inspector/PluginsWidget.moc.h>
#include <Inspector/ReflectionWidget.moc.h>
#include <Inspector/ResourceWidget.moc.h>
#include <Inspector/SubsystemsWidget.moc.h>
#include <Inspector/TimeWidget.moc.h>

void plQtMainWindow::on_ActionShowWindowLog_triggered()
{
  plQtLogDockWidget::s_pWidget->toggleView(ActionShowWindowLog->isChecked());
  plQtLogDockWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowMemory_triggered()
{
  plQtMemoryWidget::s_pWidget->toggleView(ActionShowWindowMemory->isChecked());
  plQtMemoryWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowTime_triggered()
{
  plQtTimeWidget::s_pWidget->toggleView(ActionShowWindowTime->isChecked());
  plQtTimeWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowInput_triggered()
{
  plQtInputWidget::s_pWidget->toggleView(ActionShowWindowInput->isChecked());
  plQtInputWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowCVar_triggered()
{
  plQtCVarsWidget::s_pWidget->toggleView(ActionShowWindowCVar->isChecked());
  plQtCVarsWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowReflection_triggered()
{
  plQtReflectionWidget::s_pWidget->toggleView(ActionShowWindowReflection->isChecked());
  plQtReflectionWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowSubsystems_triggered()
{
  plQtSubsystemsWidget::s_pWidget->toggleView(ActionShowWindowSubsystems->isChecked());
  plQtSubsystemsWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowPlugins_triggered()
{
  plQtPluginsWidget::s_pWidget->toggleView(ActionShowWindowPlugins->isChecked());
  plQtPluginsWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowFile_triggered()
{
  plQtFileWidget::s_pWidget->toggleView(ActionShowWindowFile->isChecked());
  plQtFileWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowGlobalEvents_triggered()
{
  plQtGlobalEventsWidget::s_pWidget->toggleView(ActionShowWindowGlobalEvents->isChecked());
  plQtGlobalEventsWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowData_triggered()
{
  plQtDataWidget::s_pWidget->toggleView(ActionShowWindowData->isChecked());
  plQtDataWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionShowWindowResource_triggered()
{
  plQtResourceWidget::s_pWidget->toggleView(ActionShowWindowResource->isChecked());
  plQtResourceWidget::s_pWidget->raise();
}

void plQtMainWindow::on_ActionOnTopWhenConnected_triggered()
{
  SetAlwaysOnTop(WhenConnected);
}

void plQtMainWindow::on_ActionAlwaysOnTop_triggered()
{
  SetAlwaysOnTop(Always);
}

void plQtMainWindow::on_ActionNeverOnTop_triggered()
{
  SetAlwaysOnTop(Never);
}
