#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QUrl>

PLASMA_IMPLEMENT_SINGLETON(plQtUiServices);

plEvent<const plQtUiServices::Event&> plQtUiServices::s_Events;
plEvent<const plQtUiServices::TickEvent&> plQtUiServices::s_TickEvent;

plMap<plString, QIcon> plQtUiServices::s_IconsCache;
plMap<plString, QImage> plQtUiServices::s_ImagesCache;
plMap<plString, QPixmap> plQtUiServices::s_PixmapsCache;
bool plQtUiServices::s_bHeadless;
plQtUiServices::TickEvent plQtUiServices::s_LastTickEvent;

static plQtUiServices* g_pInstance = nullptr;

// clang-format off
PLASMA_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtUiServices)

  ON_CORESYSTEMS_STARTUP
  {
    g_pInstance = PLASMA_DEFAULT_NEW(plQtUiServices);
    plQtUiServices::GetSingleton()->Init();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    PLASMA_DEFAULT_DELETE(g_pInstance);
  }

PLASMA_END_SUBSYSTEM_DECLARATION;
// clang-format on

plQtUiServices::plQtUiServices()
  : m_SingletonRegistrar(this)
{
  int id = qRegisterMetaType<plUuid>();
  m_pColorDlg = nullptr;
}


bool plQtUiServices::IsHeadless()
{
  return s_bHeadless;
}


void plQtUiServices::SetHeadless(bool bHeadless)
{
  s_bHeadless = true;
}

void plQtUiServices::SaveState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    Settings.setValue("ColorDlgGeom", m_ColorDlgGeometry);
  }
  Settings.endGroup();
}

plMutex m;

const QIcon& plQtUiServices::GetCachedIconResource(const char* szIdentifier, plColor color)
{
  plStringBuilder sIdentifier = szIdentifier;
  auto& map = s_IconsCache;

  if (color != plColor::ZeroColor())
  {
    sIdentifier.AppendFormat("-{}", plColorGammaUB(color));
  }

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  plStringBuilder filename = sIdentifier.GetFileName();

  if (color != plColor::ZeroColor())
  {
    PLASMA_LOCK(m);

    const plColorGammaUB color8 = color;

    plOSFile::CreateDirectoryStructure(plOSFile::GetTempDataFolder("QIcons")).AssertSuccess();
    const plStringBuilder name(plOSFile::GetTempDataFolder("QIcons"), "/", filename, ".svg");

    QFile file(szIdentifier);
    file.open(QIODeviceBase::OpenModeFlag::ReadOnly);
    QByteArray content = file.readAll();
    file.close();

    QString sContent = content;

    const QChar c0 = '0';

    sContent = sContent.replace("#ffffff", QString("#%1%2%3").arg((int)color8.r, 2, 16, c0).arg((int)color8.g, 2, 16, c0).arg((int)color8.b, 2, 16, c0), Qt::CaseInsensitive);

    {
      plStringBuilder tmp = sContent.toUtf8().data();

      QFile fileOut(name.GetData());
      fileOut.open(QIODeviceBase::OpenModeFlag::WriteOnly);
      fileOut.write(tmp.GetData(), tmp.GetElementCount());
      fileOut.flush();
      fileOut.close();
    }

    QIcon icon(name.GetData());

    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sIdentifier] = icon;
    else
      map[sIdentifier] = QIcon();
  }
  else
  {
    QIcon icon(szIdentifier);

    // Workaround for QIcon being stupid and treating failed to load icons as not-null.
    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sIdentifier] = icon;
    else
      map[sIdentifier] = QIcon();
  }

  return map[sIdentifier];
}


const QImage& plQtUiServices::GetCachedImageResource(const char* szIdentifier)
{
  const plString sIdentifier = szIdentifier;
  auto& map = s_ImagesCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QImage(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

const QPixmap& plQtUiServices::GetCachedPixmapResource(const char* szIdentifier)
{
  const plString sIdentifier = szIdentifier;
  auto& map = s_PixmapsCache;

  auto it = map.Find(sIdentifier);

  if (it.IsValid())
    return it.Value();

  map[sIdentifier] = QPixmap(QString::fromUtf8(szIdentifier));

  return map[sIdentifier];
}

plResult plQtUiServices::AddToGitIgnore(const char* szGitIgnoreFile, const char* szPattern)
{
  plStringBuilder ignoreFile;

  {
    plFileReader file;
    if (file.Open(szGitIgnoreFile).Succeeded())
    {
      ignoreFile.ReadAll(file);
    }
  }

  ignoreFile.Trim("\n\r");

  const plUInt32 len = plStringUtils::GetStringElementCount(szPattern);

  // pattern already present ?
  if (const char* szFound = ignoreFile.FindSubString(szPattern))
  {
    if (szFound == ignoreFile.GetData() || // right at the start
        *(szFound - 1) == '\n')            // after a new line
    {
      const char end = *(szFound + len);

      if (end == '\0' || end == '\r' || end == '\n') // line does not continue with an extended pattern
      {
        return PLASMA_SUCCESS;
      }
    }
  }

  ignoreFile.AppendWithSeparator("\n", szPattern);
  ignoreFile.Append("\n\n");

  {
    plFileWriter file;
    PLASMA_SUCCEED_OR_RETURN(file.Open(szGitIgnoreFile));

    PLASMA_SUCCEED_OR_RETURN(file.WriteBytes(ignoreFile.GetData(), ignoreFile.GetElementCount()));
  }

  return PLASMA_SUCCESS;
}

void plQtUiServices::CheckForUpdates()
{
  Event e;
  e.m_Type = Event::Type::CheckForUpdates;
  s_Events.Broadcast(e);
}

void plQtUiServices::Init()
{
  s_LastTickEvent.m_fRefreshRate = 60.0;
  if (QScreen* pScreen = QApplication::primaryScreen())
  {
    s_LastTickEvent.m_fRefreshRate = pScreen->refreshRate();
  }

  QTimer::singleShot(0, this, SLOT(TickEventHandler()));
}

void plQtUiServices::TickEventHandler()
{
  PLASMA_PROFILE_SCOPE("TickEvent");

  PLASMA_ASSERT_DEV(!m_bIsDrawingATM, "Implementation error");
  plTime startTime = plTime::Now();

  m_bIsDrawingATM = true;
  s_LastTickEvent.m_uiFrame++;
  s_LastTickEvent.m_Time = startTime;
  s_LastTickEvent.m_Type = TickEvent::Type::StartFrame;
  s_TickEvent.Broadcast(s_LastTickEvent);

  s_LastTickEvent.m_Type = TickEvent::Type::EndFrame;
  s_TickEvent.Broadcast(s_LastTickEvent);
  m_bIsDrawingATM = false;

  const plTime endTime = plTime::Now();
  plTime lastFrameTime = endTime - startTime;

  plTime delay = plTime::Milliseconds(1000.0 / s_LastTickEvent.m_fRefreshRate);
  delay -= lastFrameTime;
  delay = plMath::Max(delay, plTime::Zero());

  QTimer::singleShot(0, this, SLOT(TickEventHandler()));
}

void plQtUiServices::LoadState()
{
  QSettings Settings;
  Settings.beginGroup("EditorGUI");
  {
    m_ColorDlgGeometry = Settings.value("ColorDlgGeom").toByteArray();
  }
  Settings.endGroup();
}

void plQtUiServices::ShowAllDocumentsTemporaryStatusBarMessage(const plFormatString& msg, plTime timeOut)
{
  plStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowDocumentTemporaryStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = timeOut;

  s_Events.Broadcast(e, 1);
}

void plQtUiServices::ShowAllDocumentsPermanentStatusBarMessage(const plFormatString& msg, Event::TextType type)
{
  plStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowDocumentPermanentStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_TextType = type;

  s_Events.Broadcast(e, 1);
}

void plQtUiServices::ShowGlobalStatusBarMessage(const plFormatString& msg)
{
  plStringBuilder tmp;

  Event e;
  e.m_Type = Event::ShowGlobalStatusBarText;
  e.m_sText = msg.GetText(tmp);
  e.m_Time = plTime::Seconds(0);

  s_Events.Broadcast(e);
}


bool plQtUiServices::OpenFileInDefaultProgram(const char* szPath)
{
  return QDesktopServices::openUrl(QUrl::fromLocalFile(szPath));
}

void plQtUiServices::OpenInExplorer(const char* szPath, bool bIsFile)
{
  QStringList args;

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  if (bIsFile)
    args << "/select,";

  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("explorer", args);
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
  plStringBuilder parentDir;

  if (bIsFile)
  {
    parentDir = szPath;
    parentDir = parentDir.GetFileDirectory();
    szPath = parentDir.GetData();
  }
  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("xdg-open", args);
#else
  PLASMA_ASSERT_NOT_IMPLEMENTED
#endif
}

plStatus plQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe =
    QStandardPaths::locate(QStandardPaths::GenericDataLocation, "Programs/Microsoft VS Code/Code.exe", QStandardPaths::LocateOption::LocateFile);

  if (!QFile().exists(sVsCodeExe))
  {
    QSettings settings("\\HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Applications\\Code.exe\\shell\\open\\command", QSettings::NativeFormat);
    QString sVsCodeExeKey = settings.value(".", "").value<QString>();

    if (sVsCodeExeKey.length() > 5)
    {
      // Remove shell parameter and normalize QT Compatible path, QFile expects the file separator to be '/' regardless of operating system
      sVsCodeExe = sVsCodeExeKey.left(sVsCodeExeKey.length() - 5).replace("\\", "/").replace("\"", "");
    }
  }

  if (!QFile().exists(sVsCodeExe))
  {
    return plStatus("Installation of Visual Studio Code could not be located.\n"
                    "Please visit 'https://code.visualstudio.com/download' to download the 'User Installer' of Visual Studio Code.");
  }

  QProcess proc;
  if (proc.startDetached(sVsCodeExe, arguments) == false)
  {
    return plStatus("Failed to launch Visual Studio Code.");
  }

  return plStatus(PLASMA_SUCCESS);
}
