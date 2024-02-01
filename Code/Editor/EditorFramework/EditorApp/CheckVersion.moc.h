#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Strings/String.h>

#include <Foundation/Types/UniquePtr.h>
#include <QObject>
#include <QProcess>

class PageDownloader : public QObject
{
  Q_OBJECT

public:
  explicit PageDownloader(const QString& sUrl);

  plStringView GetDownloadedData() const { return m_sDownloadedPage; }

signals:
  void FinishedDownload();

private slots:
  void DownloadDone(int exitCode, QProcess::ExitStatus exitStatus);

private:
  plUniquePtr<QProcess> m_pProcess;
  plStringBuilder m_sDownloadedPage;
};

/// \brief Downloads a web page and checks whether the latest version online is newer than the current one
class plQtVersionChecker : public QObject
{
  Q_OBJECT

public:
  plQtVersionChecker();

  void Initialize();

  bool Check(bool bForce);

  const char* GetOwnVersion() const;
  const char* GetKnownLatestVersion() const;

  bool IsLatestNewer() const;

Q_SIGNALS:
  void VersionCheckCompleted(bool bNewRelease, bool bForced);

private slots:
  void PageDownloaded();

  plResult StoreKnownVersion();

private:
  bool m_bRequireOnlineCheck = true;
  bool m_bForceCheck = false;
  bool m_bCheckInProgresss = false;
  plString m_sConfigFile;
  plString m_sKnownLatestVersion;
  plUniquePtr<PageDownloader> m_pVersionPage;
};
