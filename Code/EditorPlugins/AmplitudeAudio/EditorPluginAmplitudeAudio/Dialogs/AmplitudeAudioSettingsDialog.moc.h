#pragma once

#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioDLL.h>
#include <EditorPluginAmplitudeAudio/ui_AmplitudeAudioSettingsDialog.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioSingleton.h>

#include <QDialog>

class plQtAmplitudeAudioSettingsDialog : public QDialog, public Ui_AmplitudeAudioSettingsDialog
{
public:
  Q_OBJECT

public:
  plQtAmplitudeAudioSettingsDialog(QWidget* parent);

private Q_SLOTS:
  void on_ButtonBox_clicked(QAbstractButton* pButton);
  void on_ListPlatforms_itemSelectionChanged();
  void on_ButtonAdd_clicked();
  void on_ButtonRemove_clicked();

private:
  plResult Save();
  void Load();
  void SetCurrentPlatform(const char* szPlatform);
  void StoreCurrentPlatform();

  plString m_sCurrentPlatform;
  plAmplitudeAssetProfiles m_ConfigsOld;
  plAmplitudeAssetProfiles m_Configs;
};
