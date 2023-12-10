#pragma once

#include <EditorFramework/ui_SnapSettingsDlg.h>
#include <Foundation/Containers/HybridArray.h>

class QAbstractButton;

class plQtSnapSettingsDlg : public QDialog, public Ui_SnapSettingsDlg
{
  Q_OBJECT

public:
  plQtSnapSettingsDlg(QWidget* pParent);

private Q_SLOTS:
  void on_ButtonBox_clicked(QAbstractButton* button);

private:
  struct KeyValue
  {
    PLASMA_DECLARE_POD_TYPE();

    const char* m_szKey;
    float m_fValue;
  };

  plHybridArray<KeyValue, 16> m_Translation;
  plHybridArray<KeyValue, 16> m_Rotation;
  plHybridArray<KeyValue, 16> m_Scale;

  void QueryUI();
};

