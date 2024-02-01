#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Stopwatch.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <QDesktopServices>
#include <QDir>
#include <QIcon>
#include <QProcess>
#include <QScreen>
#include <QSettings>
#include <QUrl>

PL_IMPLEMENT_SINGLETON(plQtUiServices);

plEvent<const plQtUiServices::Event&> plQtUiServices::s_Events;
plEvent<const plQtUiServices::TickEvent&> plQtUiServices::s_TickEvent;

plMap<plString, QIcon> plQtUiServices::s_IconsCache;
plMap<plString, QImage> plQtUiServices::s_ImagesCache;
plMap<plString, QPixmap> plQtUiServices::s_PixmapsCache;
bool plQtUiServices::s_bHeadless;
plQtUiServices::TickEvent plQtUiServices::s_LastTickEvent;

static plQtUiServices* g_pInstance = nullptr;

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(GuiFoundation, QtUiServices)

  ON_CORESYSTEMS_STARTUP
  {
    g_pInstance = PL_DEFAULT_NEW(plQtUiServices);
    plQtUiServices::GetSingleton()->Init();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    PL_DEFAULT_DELETE(g_pInstance);
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plQtUiServices::plQtUiServices()
  : m_SingletonRegistrar(this)
{
  qRegisterMetaType<plUuid>();
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

plTime g_Total = plTime::MakeZero();

const QIcon& plQtUiServices::GetCachedIconResource(plStringView sIdentifier, plColor svgTintColor)
{
  plStringBuilder sFullIdentifier = sIdentifier;
  auto& map = s_IconsCache;

  const bool bNeedsColoring = svgTintColor != plColor::MakeZero() && sIdentifier.EndsWith_NoCase(".svg");

  if (bNeedsColoring)
  {
    sFullIdentifier.AppendFormat("-{}", plColorGammaUB(svgTintColor));
  }

  auto it = map.Find(sFullIdentifier);

  if (it.IsValid())
    return it.Value();

  if (bNeedsColoring)
  {
    plStopwatch sw;

    // read the icon from the Qt virtual file system (QResource)
    QFile file(plString(sIdentifier).GetData());
    if (!file.open(QIODeviceBase::OpenModeFlag::ReadOnly))
    {
      // if it doesn't exist, return an empty QIcon

      map[sFullIdentifier] = QIcon();
      return map[sFullIdentifier];
    }

    // get the entire SVG file content
    plStringBuilder sContent = QString(file.readAll()).toUtf8().data();

    // replace the occurrence of the color white ("#FFFFFF") with the desired target color
    {
      const plColorGammaUB color8 = svgTintColor;

      plStringBuilder rep;
      rep.SetFormat("#{}{}{}", plArgI((int)color8.r, 2, true, 16), plArgI((int)color8.g, 2, true, 16), plArgI((int)color8.b, 2, true, 16));

      sContent.ReplaceAll_NoCase("#ffffff", rep);

      rep.Append(";");
      sContent.ReplaceAll_NoCase("#fff;", rep);
      rep.Shrink(0, 1);

      rep.Prepend("\"");
      rep.Append("\"");
      sContent.ReplaceAll_NoCase("\"#fff\"", rep);
    }

    // hash the content AFTER the color replacement, so it includes the custom color change
    const plUInt32 uiSrcHash = plHashingUtils::xxHash32String(sContent);

    // file the path to the temp file, including the source hash
    const plStringBuilder sTempFolder = plOSFile::GetTempDataFolder("plEditor/QIcons");
    plStringBuilder sTempIconFile(sTempFolder, "/", sIdentifier.GetFileName());
    sTempIconFile.AppendFormat("-{}.svg", uiSrcHash);

    // only write to the file system, if the target file doesn't exist yet, this saves more than half the time
    if (!plOSFile::ExistsFile(sTempIconFile))
    {
      // now write the new SVG file back to a dummy file
      // yes, this is as stupid as it sounds, we really write the file BACK TO THE FILESYSTEM, rather than doing this stuff in-memory
      // that's because I wasn't able to figure out whether we can somehow read a QIcon from a string rather than from file
      // it doesn't appear to be easy at least, since we can only give it a path, not a memory stream or anything like that
      {
        // necessary for Qt to be able to write to the folder
        plOSFile::CreateDirectoryStructure(sTempFolder).AssertSuccess();

        QFile fileOut(sTempIconFile.GetData());
        fileOut.open(QIODeviceBase::OpenModeFlag::WriteOnly);
        fileOut.write(sContent.GetData(), sContent.GetElementCount());
        fileOut.flush();
        fileOut.close();
      }
    }

    QIcon icon(sTempIconFile.GetData());

    if (!icon.pixmap(QSize(16, 16)).isNull())
      map[sFullIdentifier] = icon;
    else
      map[sFullIdentifier] = QIcon();

    plTime local = sw.GetRunningTotal();
    g_Total += local;

    // kept here for debug purposes, but don't waste time on logging
    // plLog::Info("Icon load time: {}, total = {}", local, g_Total);
  }
  else
  {
    const QString sFile = plString(sIdentifier).GetData();

    if (QFile::exists(sFile)) // prevent Qt from spamming warnings about non-existing files by checking this manually
    {
      QIcon icon(sFile);

      // Workaround for QIcon being stupid and treating failed to load icons as not-null.
      if (!icon.pixmap(QSize(16, 16)).isNull())
        map[sFullIdentifier] = icon;
      else
        map[sFullIdentifier] = QIcon();
    }
    else
      map[sFullIdentifier] = QIcon();
  }

  return map[sFullIdentifier];
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
        return PL_SUCCESS;
      }
    }
  }

  ignoreFile.AppendWithSeparator("\n", szPattern);
  ignoreFile.Append("\n\n");

  {
    plFileWriter file;
    PL_SUCCEED_OR_RETURN(file.Open(szGitIgnoreFile));

    PL_SUCCEED_OR_RETURN(file.WriteBytes(ignoreFile.GetData(), ignoreFile.GetElementCount()));
  }

  return PL_SUCCESS;
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

  QTimer::singleShot((plInt32)plMath::Floor(1000.0 / s_LastTickEvent.m_fRefreshRate), this, SLOT(TickEventHandler()));
}

void plQtUiServices::TickEventHandler()
{
  PL_PROFILE_SCOPE("TickEvent");

  PL_ASSERT_DEV(!m_bIsDrawingATM, "Implementation error");
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

  plTime delay = plTime::MakeFromMilliseconds(1000.0 / s_LastTickEvent.m_fRefreshRate);
  delay -= lastFrameTime;
  delay = plMath::Max(delay, plTime::MakeZero());

  QTimer::singleShot((plInt32)plMath::Floor(delay.GetMilliseconds()), this, SLOT(TickEventHandler()));
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
  e.m_Time = plTime::MakeFromSeconds(0);

  s_Events.Broadcast(e);
}


bool plQtUiServices::OpenFileInDefaultProgram(const char* szPath)
{
  return QDesktopServices::openUrl(QUrl::fromLocalFile(szPath));
}

void plQtUiServices::OpenInExplorer(const char* szPath, bool bIsFile)
{
  QStringList args;

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  if (bIsFile)
    args << "/select,";

  args << QDir::toNativeSeparators(szPath);

  QProcess::startDetached("explorer", args);
#elif PL_ENABLED(PL_PLATFORM_LINUX)
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
  PL_ASSERT_NOT_IMPLEMENTED
#endif
}

plStatus plQtUiServices::OpenInVsCode(const QStringList& arguments)
{
  QString sVsCodeExe;
#if PL_ENABLED(PL_PLATFORM_WINDOWS)
  sVsCodeExe =
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
#endif

  if(sVsCodeExe.isEmpty() || !QFile().exists(sVsCodeExe))
  {
    // Try code executable in PATH
    if(QProcess::execute("code", {"--version"}) == 0)
    {
      sVsCodeExe = "code";
    }
    else
    {
      return plStatus("Installation of Visual Studio Code could not be located.\n"
                      "Please visit 'https://code.visualstudio.com/download' to download the 'User Installer' of Visual Studio Code.");
    }
  }

  QProcess proc;
  if (proc.startDetached(sVsCodeExe, arguments) == false)
  {
    return plStatus("Failed to launch Visual Studio Code.");
  }

  return plStatus(PL_SUCCESS);
}
