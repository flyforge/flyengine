#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/CheckVersion.moc.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <QNetworkReply>
#include <QProcess>

static plString GetVersionFilePath()
{
  plStringBuilder sTemp = plOSFile::GetTempDataFolder();
  sTemp.AppendPath("plEditor/version-page.htm");
  return sTemp;
}

PageDownloader::PageDownloader(const QString& sUrl)
{
#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  QStringList args;

  args << "-Command";
  args << QString("(Invoke-webrequest -URI \"%1\").Content > \"%2\"").arg(sUrl).arg(GetVersionFilePath().GetData());

  m_pProcess = PL_DEFAULT_NEW(QProcess);
  connect(m_pProcess.Borrow(), &QProcess::finished, this, &PageDownloader::DownloadDone);
  m_pProcess->start("C:\\Windows\\System32\\WindowsPowershell\\v1.0\\powershell.exe", args);
#else
  PL_ASSERT_NOT_IMPLEMENTED;
#endif
}

void PageDownloader::DownloadDone(int exitCode, QProcess::ExitStatus exitStatus)
{
  m_pProcess = nullptr;

  plOSFile file;
  if (file.Open(GetVersionFilePath(), plFileOpenMode::Read).Failed())
    return;

  plDataBuffer content;
  file.ReadAll(content);

  content.PushBack('\0');
  content.PushBack('\0');

  const plUInt16* pStart = (plUInt16*)content.GetData();
  if (plUnicodeUtils::SkipUtf16BomLE(pStart))
  {
    m_sDownloadedPage = plStringWChar(pStart);
  }
  else
  {
    const char* szUtf8 = (const char*)content.GetData();
    m_sDownloadedPage = plStringWChar(szUtf8);
  }

  plOSFile::DeleteFile(GetVersionFilePath()).IgnoreResult();

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

  const plTimestamp nextCheck = fs.m_LastModificationTime + plTime::MakeFromHours(24);

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
    return PL_FAILURE;

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);
  plOpenDdlUtils::StoreString(ddl, m_sKnownLatestVersion, "KnownLatest");

  return PL_SUCCESS;
}

bool plQtVersionChecker::Check(bool bForce)
{
#if PL_DISABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  PL_ASSERT_DEV(!bForce, "The version check is not yet implemented on this platform.");
  return false;
#endif

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

  m_pVersionPage = PL_DEFAULT_NEW(PageDownloader, "https://plengine.net/pages/getting-started/binaries.html");

  connect(m_pVersionPage.Borrow(), &PageDownloader::FinishedDownload, this, &plQtVersionChecker::PageDownloaded);

  return true;
}

const char* plQtVersionChecker::GetOwnVersion() const
{
  return PL_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MAJOR) "." PL_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_MINOR) "." PL_PP_STRINGIFY(BUILDSYSTEM_SDKVERSION_PATCH);
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
  plStringBuilder sPage = m_pVersionPage->GetDownloadedData();

  m_pVersionPage = nullptr;

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
