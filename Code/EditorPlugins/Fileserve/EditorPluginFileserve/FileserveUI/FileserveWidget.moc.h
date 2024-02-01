#pragma once

#include <EditorPluginFileserve/EditorPluginFileserveDLL.h>
#include <EditorPluginFileserve/ui_FileserveWidget.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Time/Time.h>
#include <QWidget>

struct plFileserverEvent;
class plQtFileserveActivityModel;
class plQtFileserveAllFilesModel;
enum class plFileserveActivityType;

/// \brief A GUI for the plFileServer
///
/// By default the file server does not run at startup. Using the command line option "-fs_start" the server is started immediately.
class PL_EDITORPLUGINFILESERVE_DLL plQtFileserveWidget : public QWidget, public Ui_plQtFileserveWidget
{
  Q_OBJECT

public:
  plQtFileserveWidget(QWidget* pParent = nullptr);

  void FindOwnIP(plStringBuilder& out_sDisplay, plHybridArray<plStringBuilder, 4>* out_pAllIPs = nullptr);

  ~plQtFileserveWidget();

Q_SIGNALS:
  void ServerStarted(const QString& sIp, plUInt16 uiPort);
  void ServerStopped();

public Q_SLOTS:
  void on_StartServerButton_clicked();
  void on_ClearActivityButton_clicked();
  void on_ClearAllFilesButton_clicked();
  void on_ReloadResourcesButton_clicked();
  void on_ConnectClient_clicked();

private:
  void FileserverEventHandler(const plFileserverEvent& e);
  void LogActivity(const plFormatString& text, plFileserveActivityType type);
  void UpdateSpecialDirectoryUI();

  plQtFileserveActivityModel* m_pActivityModel;
  plQtFileserveAllFilesModel* m_pAllFilesModel;
  plTime m_LastProgressUpdate;

  struct DataDirInfo
  {
    plString m_sName;
    plString m_sPath;
    plString m_sRedirectedPath;
  };

  struct ClientData
  {
    bool m_bConnected = false;
    plHybridArray<DataDirInfo, 8> m_DataDirs;
  };

  struct SpecialDir
  {
    plString m_sName;
    plString m_sPath;
  };

  plHybridArray<SpecialDir, 4> m_SpecialDirectories;

  plHashTable<plUInt32, ClientData> m_Clients;
  void UpdateClientList();
  void ConfigureSpecialDirectories();
};

