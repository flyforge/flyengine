#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <Foundation/Strings/String.h>

#include <QByteArray>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPointer>

class PageDownloader : public QObject
{
  Q_OBJECT

public:
  explicit PageDownloader(QUrl url);

  const QByteArray& GetDownloadedData() const { return m_DownloadedData; }

signals:
  void FinishedDownload();

private slots:
  void DownloadDone(QNetworkReply* pReply);

private:
  QNetworkAccessManager m_WebCtrl;
  QByteArray m_DownloadedData;
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
  QPointer<PageDownloader> m_pVersionPage;
};

