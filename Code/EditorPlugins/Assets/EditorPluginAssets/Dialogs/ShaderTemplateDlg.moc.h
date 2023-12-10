#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/ui_ShaderTemplateDlg.h>
#include <QDialog>

class plDocument;

class plQtShaderTemplateDlg : public QDialog, public Ui_ShaderTemplateDlg
{
  Q_OBJECT

public:
  plQtShaderTemplateDlg(QWidget* pParent, const plDocument* pDoc);

  plString m_sResult;

private Q_SLOTS:
  void on_Buttons_accepted();
  void on_Buttons_rejected();
  void on_Browse_clicked();
  void on_ShaderTemplate_currentIndexChanged(int idx);

private:
  struct Template
  {
    plString m_sName;
    plString m_sPath;
    plString m_sContent;
    plHybridArray<plString, 16> m_Vars;
  };

  plHybridArray<Template, 32> m_Templates;
};
