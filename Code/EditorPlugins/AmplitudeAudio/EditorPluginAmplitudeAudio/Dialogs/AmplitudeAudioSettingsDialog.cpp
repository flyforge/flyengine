#include "Foundation/IO/FileSystem/FileWriter.h"
#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioPCH.h>

#include <EditorPluginAmplitudeAudio/Dialogs/AmplitudeAudioSettingsDialog.moc.h>

#include <AmplitudeAudioPlugin/Core/Common.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Types/Types.h>

#include <QFileDialog>
#include <QInputDialog>

plQtAmplitudeAudioSettingsDialog::plQtAmplitudeAudioSettingsDialog(QWidget* parent)
  : QDialog(parent)
{
  setupUi(this);

  plStringBuilder projectPath;
  if (plFileSystem::ResolvePath(":project/Sounds/Amplitude/amplitude_assets", &projectPath, nullptr).Failed())
  {
    plLog::Error("No Amplitude assets directory available. Cannot customize project settings.");
    return;
  }

  Load();

  for (auto it = m_Configs.m_AssetProfiles.GetIterator(); it.IsValid(); ++it)
  {
    ListPlatforms->addItem(it.Key().GetData());
  }

  plStringBuilder banksPath(projectPath);
  banksPath.AppendPath(kSoundBanksFolder);

  plFileSystemIterator fsIt;
  for (fsIt.StartSearch(projectPath, plFileSystemIteratorFlags::ReportFiles); fsIt.IsValid(); fsIt.Next())
  {
    plStringBuilder fileName = fsIt.GetStats().m_sName;
    if (fileName.HasExtension(".amconfig"))
    {
      ComboConfig->addItem(fileName.GetData());
    }
  }

  for (fsIt.StartSearch(banksPath, plFileSystemIteratorFlags::ReportFiles); fsIt.IsValid(); fsIt.Next())
  {
    plStringBuilder fileName = fsIt.GetStats().m_sName;
    if (fileName.HasExtension(".ambank"))
    {
      ComboBank->addItem(fileName.GetData());
    }
  }

  if (!m_Configs.m_AssetProfiles.IsEmpty())
  {
    if (m_Configs.m_AssetProfiles.Contains("Desktop"))
      SetCurrentPlatform("Desktop");
    else
      SetCurrentPlatform(m_Configs.m_AssetProfiles.GetIterator().Key());
  }
  else
  {
    SetCurrentPlatform("");
  }
}
plResult plQtAmplitudeAudioSettingsDialog::Save()
{
  plFileWriter file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(":project/Sounds/AudioSystemConfig.ddl"));

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("Middleware", s_szAmplitudeMiddlewareName);
  {
    if (m_Configs.Save(ddl).Failed())
    {
      plQtUiServices::GetSingleton()->MessageBoxWarning("Failed to save the Audio System configuration file\n'>project/Sounds/AudioSystemConfig.ddl'");
      return PLASMA_FAILURE;
    }
  }
  ddl.EndObject();

  return PLASMA_SUCCESS;
}

void plQtAmplitudeAudioSettingsDialog::Load()
{
  plFileReader file;
  if (file.Open(":project/Sounds/AudioSystemConfig.ddl").Failed())
    return;

  plOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return;

  const plOpenDdlReaderElement* pRoot = ddl.GetRootElement();
  const plOpenDdlReaderElement* pChild = pRoot->GetFirstChild();

  while (pChild)
  {
    if (pChild->IsCustomType("Middleware") && pChild->HasName() && pChild->GetName().Compare(s_szAmplitudeMiddlewareName) == 0)
    {
      if (m_Configs.Load(*pChild).Failed())
        plLog::Error("Failed to load configuration for audio middleware: {0}.", pChild->GetName());

      break;
    }

    pChild = pChild->GetSibling();
  }

  m_ConfigsOld = m_Configs;
}

void plQtAmplitudeAudioSettingsDialog::SetCurrentPlatform(const char* szPlatform)
{
  StoreCurrentPlatform();

  m_sCurrentPlatform = szPlatform;

  {
    const bool enable = !m_sCurrentPlatform.IsEmpty();
    ComboBank->setEnabled(enable);
    ComboConfig->setEnabled(enable);
    ButtonRemove->setEnabled(enable);
  }

  plQtScopedBlockSignals bs(ListPlatforms);
  QList<QListWidgetItem*> items = ListPlatforms->findItems(szPlatform, Qt::MatchFlag::MatchExactly);

  ListPlatforms->clearSelection();

  if (items.size() > 0)
  {
    items[0]->setSelected(true);
  }

  m_sCurrentPlatform = szPlatform;

  if (m_sCurrentPlatform.IsEmpty())
    return;

  const auto& cfg = m_Configs.m_AssetProfiles[m_sCurrentPlatform];

  ComboBank->setCurrentText(cfg.m_sInitSoundBank.GetData());
  ComboConfig->setCurrentText(cfg.m_sEngineConfigFileName.GetData());
}

void plQtAmplitudeAudioSettingsDialog::StoreCurrentPlatform()
{
  if (!m_Configs.m_AssetProfiles.Contains(m_sCurrentPlatform))
    return;

  auto& cfg = m_Configs.m_AssetProfiles[m_sCurrentPlatform];

  cfg.m_sInitSoundBank = ComboBank->currentText().toUtf8().data();
  cfg.m_sEngineConfigFileName = ComboConfig->currentText().toUtf8().data();
}

void plQtAmplitudeAudioSettingsDialog::on_ButtonBox_clicked(QAbstractButton* pButton)
{
  if (pButton == ButtonBox->button(QDialogButtonBox::Ok))
  {
    StoreCurrentPlatform();

    if (m_ConfigsOld.m_AssetProfiles != m_Configs.m_AssetProfiles)
    {
      if (plQtUiServices::GetSingleton()->MessageBoxQuestion("Save the changes to the Amplitude Audio configuration?\nYou need to reload the project for the changes to take effect.", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
      {
        plQtEditorApp::GetSingleton()->AddReloadProjectRequiredReason("Amplitude Audio configuration was modified.");

        if (Save().Failed())
          return;
      }
    }

    accept();
    return;
  }

  if (pButton == ButtonBox->button(QDialogButtonBox::Cancel))
  {
    reject();
    return;
  }
}

void plQtAmplitudeAudioSettingsDialog::on_ListPlatforms_itemSelectionChanged()
{
  if (ListPlatforms->selectedItems().isEmpty())
  {
    SetCurrentPlatform("");
    return;
  }

  ButtonRemove->setEnabled(true);
  int row = ListPlatforms->selectionModel()->selectedIndexes()[0].row();
  SetCurrentPlatform(ListPlatforms->item(row)->text().toUtf8().data());
}

void plQtAmplitudeAudioSettingsDialog::on_ButtonAdd_clicked()
{
  QString name = QInputDialog::getText(this, "Add Platform", "Platform Name:");

  if (name.isEmpty())
    return;

  const plString sName = name.toUtf8().data();

  if (!m_Configs.m_AssetProfiles.Contains(sName))
  {
    // add a new item with default values
    m_Configs.m_AssetProfiles[sName];

    ListPlatforms->addItem(sName.GetData());
  }

  SetCurrentPlatform(sName);
}

void plQtAmplitudeAudioSettingsDialog::on_ButtonRemove_clicked()
{
  if (ListPlatforms->selectedItems().isEmpty())
    return;

  int row = ListPlatforms->selectionModel()->selectedIndexes()[0].row();
  const plString sPlatform = ListPlatforms->item(row)->text().toUtf8().data();

  m_Configs.m_AssetProfiles.Remove(sPlatform);
  delete ListPlatforms->item(row);
}
