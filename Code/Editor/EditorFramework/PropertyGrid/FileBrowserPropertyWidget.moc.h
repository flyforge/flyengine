#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>
#include <QLineEdit>

class PLASMA_EDITORFRAMEWORK_DLL plQtFilePropertyWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtFilePropertyWidget();

private Q_SLOTS:
  void on_BrowseFile_clicked();

protected slots:
  void on_TextFinished_triggered();
  void on_TextChanged_triggered(const QString& value);
  void OnOpenExplorer();
  void OnCustomAction();
  void OnOpenFile();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QLineEdit* m_pWidget;
  QToolButton* m_pButton;
};

