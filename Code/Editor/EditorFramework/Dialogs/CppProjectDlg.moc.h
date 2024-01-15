
#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/ui_CppProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class PLASMA_EDITORFRAMEWORK_DLL plQtCppProjectDlg : public QDialog, public Ui_plQtCppProjectDlg
{
public:
  Q_OBJECT

public:
  plQtCppProjectDlg(QWidget* parent);

private Q_SLOTS:
  void on_Result_rejected();
  void on_OpenPluginLocation_clicked();
  void on_OpenBuildFolder_clicked();
  void on_OpenSolution_clicked();
  void on_GenerateSolution_clicked();
  void on_PluginName_textEdited(const QString& text);

private:
  void UpdateUI();

  plCppSettings m_OldCppSettings;
  plCppSettings m_CppSettings;
};
