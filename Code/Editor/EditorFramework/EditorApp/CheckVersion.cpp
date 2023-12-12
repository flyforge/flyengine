#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/CheckVersion.moc.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <QNetworkReply>

PageDownloader::PageDownloader(QUrl url)
{
  connect(&m_WebCtrl, &QNetworkAccessManager::finished, this, &PageDownloader::DownloadDone);

  QNetworkRequest request(url);
  m_WebCtrl.get(request);
}

void PageDownloader::DownloadDone(QNetworkReply* pReply)
{
  QNetworkReply::NetworkError e = pReply->error();

  if (e != QNetworkReply::NetworkError::NoError)
  {
    m_DownloadedData = pReply->readAll();
  }
  else
  {
    m_DownloadedData = pReply->readAll();
  }

  pReply->deleteLater();

  Q_EMIT FinishedDownload();
}

plQtVersionChecker::plQtVersionChecker()
{
  m_sKnownLatestVersion = GetOwnVersion();
  m_sConfigFile = ":appdata/VersionCheck.ddl";
}

void plQtVersionChecker::Initialize()
{
  m_bRequireOnlineCheck = true;

  plFileStats fs;
  if (plFileSystem::GetFileStats(m_sConfigFile, fs).Failed())
    return;

  plFileReader file;
  if (file.Open(m_sConfigFile).Failed())
    return;

  plOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return;

  auto pRoot = ddl.GetRootElement();
  if (pRoot == nullptr)
    return;

  auto pLatest = pRoot->FindChild("KnownLatest");
  if (pLatest == nullptr || !pLatest->HasPrimitives(plOpenDdlPrimitiveType::String))
    return;

  m_sKnownLatestVersion = pLatest->GetPrimitivesString()[0];

  const plTimestamp nextCheck = fs.m_LastModificationTime + plTime::Hours(24);

  if (nextCheck.Compare(plTimestamp::CurrentTimestamp(), plTimestamp::CompareMode::Newer))
  {
    // everything fine, we already checked within the last 24 hours

    m_bRequireOnlineCheck = false;
    return;
  }
}

plResult plQtVersionChecker::StoreKnownVersion()
{
  plFileWriter file;
  if (file.Open(m_sConfigFile).Failed())
    return PLASMA_FAILURE;

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);
  plOpenDdlUtils::StoreString(ddl, m_sKnownLatestVersion, "KnownLatest");

  return PLASMA_SUCCESS;
}

bool plQtVersionChecker::Check(bool bForce)
{
  if (bForce)
  {
    // to trigger a 'new release available' signal
    m_sKnownLatestVersion = GetOwnVersion();
    m_bForceCheck = true;
  }

  if (m_bCheckInProgresss)
    return false;

  if (!bForce && !m_bRequireOnlineCheck)
  {
    Q_EMIT VersionCheckCompleted(false, false);
    return false;
  }

  m_bCheckInProgresss = true;

  // DON'T use HTTPS here, our Qt version only supports HTTP
  m_pVersionPage = new PageDownloader(QUrl("http://plengine.net/pages/getting-started/binaries.html"));

  connect(m_pVersionPage.data(), &PageDownloader::FinishedDownload, this, &plQtVersionChecker::PageDownloaded);

  return true;
}

const char* plQtVersionChecker::GetOwnVersion() const
{
  return BUILDSYSTEM_SDKVERSION_MAJOR "." BUILDSYSTEM_SDKVERSION_MINOR "." BUILDSYSTEM_SDKVERSION_PATCH;
}

const char* plQtVersionChecker::GetKnownLatestVersion() const
{
  return m_sKnownLatestVersion;
}

bool plQtVersionChecker::IsLatestNewer() const
{
  const char* szParsePos;
  plUInt32 own[3] = {0, 0, 0};
  plUInt32 cur[3] = {0, 0, 0};

  szParsePos = GetOwnVersion();
  for (plUInt32 i : {0, 1, 2})
  {
    if (plConversionUtils::StringToUInt(szParsePos, own[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  szParsePos = GetKnownLatestVersion();
  for (plUInt32 i : {0, 1, 2})
  {
    if (plConversionUtils::StringToUInt(szParsePos, cur[i], &szParsePos).Failed())
      break;

    if (*szParsePos == '.')
      ++szParsePos;
    else
      break;
  }

  // 'major'
  if (own[0] > cur[0])
    return false;
  if (own[0] < cur[0])
    return true;

  // 'minor'
  if (own[1] > cur[1])
    return false;
  if (own[1] < cur[1])
    return true;

  // 'patch'
  return own[2] < cur[2];
}

void plQtVersionChecker::PageDownloaded()
{
  m_bCheckInProgresss = false;
  plStringBuilder sPage = m_pVersionPage->GetDownloadedData().data();

  if (sPage.IsEmpty())
  {
    plLog::Warning("Could not download release notes page.");
    return;
  }

  const char* szVersionStartTag = "<!--<VERSION>-->";
  const char* szVersionEndTag = "<!--</VERSION>-->";

  const char* pVersionStart = sPage.FindSubString(szVersionStartTag);
  const char* pVersionEnd = sPage.FindSubString(szVersionEndTag, pVersionStart);

  if (pVersionStart == nullptr || pVersionEnd == nullptr)
  {
    plLog::Warning("Version check failed.");
    return;
  }

  plStringBuilder sVersion;
  sVersion.SetSubString_FromTo(pVersionStart + plStringUtils::GetStringElementCount(szVersionStartTag), pVersionEnd);

  const bool bNewRelease = m_sKnownLatestVersion != sVersion;

  // make sure to modify the file, even if the version is the same
  m_sKnownLatestVersion = sVersion;
  if (StoreKnownVersion().Failed())
  {
    plLog::Warning("Could not store the last known version file.");
  }

  Q_EMIT VersionCheckCompleted(bNewRelease, m_bForceCheck);
}
