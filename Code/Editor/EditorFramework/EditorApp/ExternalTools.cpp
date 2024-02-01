#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

plString plQtEditorApp::FindToolApplication(const char* szToolName)
{
  plStringBuilder toolExe = szToolName;

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szToolName = toolExe;

  plEditorPreferencesUser* pPref = plPreferences::QueryPreferences<plEditorPreferencesUser>();

  plHybridArray<plString, 3> sFolders;
  sFolders.PushBack(plApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(false));
  sFolders.PushBack(plApplicationServices::GetSingleton()->GetPrecompiledToolsFolder(true));

  if (pPref->m_bUsePrecompiledTools)
  {
    if(!pPref->m_sCustomPrecompiledToolsFolder.IsEmpty() && plOSFile::ExistsDirectory(pPref->m_sCustomPrecompiledToolsFolder))
    {
      plStringBuilder customToolsFolder = pPref->m_sCustomPrecompiledToolsFolder;
      customToolsFolder.MakeCleanPath();
      sFolders.PushBack(customToolsFolder);
      plMath::Swap(sFolders[0], sFolders[2]);
    }
    else
    {
      plMath::Swap(sFolders[0], sFolders[1]);
    }
  }

  plStringBuilder sTool;
  for(auto& folder : sFolders)
  {
    sTool = folder;
    sTool.AppendPath(szToolName);

    if (plOSFile::ExistsFile(sTool))
      return sTool;
  }

  // just try the one in the same folder as the editor
  return szToolName;
}

plStatus plQtEditorApp::ExecuteTool(const char* szTool, const QStringList& arguments, plUInt32 uiSecondsTillTimeout, plLogInterface* pLogOutput /*= nullptr*/, plLogMsgType::Enum logLevel /*= plLogMsgType::InfoMsg*/, const char* szCWD /*= nullptr*/)
{
  // this block is supposed to be in the global log, not the given log interface
  PL_LOG_BLOCK("Executing Tool", szTool);

  plStringBuilder toolExe = szTool;

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  toolExe.ChangeFileExtension("exe");
#else
  toolExe.RemoveFileExtension();
#endif

  szTool = toolExe;

  plStringBuilder cmd;
  for (plInt32 i = 0; i < arguments.size(); ++i)
    cmd.Append(" ", arguments[i].toUtf8().data());

  plLog::Debug("{}{}", szTool, cmd);


  QProcess proc;

  if (szCWD != nullptr)
  {
    proc.setWorkingDirectory(szCWD);
  }

  QString logoutput;
  proc.setProcessChannelMode(QProcess::MergedChannels);
  proc.setReadChannel(QProcess::StandardOutput);
  QObject::connect(&proc, &QProcess::readyReadStandardOutput, [&proc, &logoutput]()
    { logoutput.append(proc.readAllStandardOutput()); });
  plString toolPath = plQtEditorApp::GetSingleton()->FindToolApplication(szTool);
  proc.start(QString::fromUtf8(toolPath, toolPath.GetElementCount()), arguments);

  if (!proc.waitForStarted(uiSecondsTillTimeout * 1000))
    return plStatus(plFmt("{0} could not be started", szTool));

  if (!proc.waitForFinished(uiSecondsTillTimeout * 1000))
    return plStatus(plFmt("{0} timed out", szTool));

  if (pLogOutput)
  {
    plStringBuilder tmp;

    struct LogBlockData
    {
      LogBlockData(plLogInterface* pInterface, const char* szName)
        : m_Name(szName)
        , m_Block(pInterface, m_Name)
      {
      }

      plString m_Name;
      plLogBlock m_Block;
    };

    plHybridArray<plUniquePtr<LogBlockData>, 8> blocks;

    QTextStream logoutputStream(&logoutput);
    while (!logoutputStream.atEnd())
    {
      tmp = logoutputStream.readLine().toUtf8().data();
      tmp.Trim(" \n");

      const char* szMsg = nullptr;
      plLogMsgType::Enum msgType = plLogMsgType::None;

      if (tmp.StartsWith("Error: "))
      {
        szMsg = &tmp.GetData()[7];
        msgType = plLogMsgType::ErrorMsg;
      }
      else if (tmp.StartsWith("Warning: "))
      {
        szMsg = &tmp.GetData()[9];
        msgType = plLogMsgType::WarningMsg;
      }
      else if (tmp.StartsWith("Seriously: "))
      {
        szMsg = &tmp.GetData()[11];
        msgType = plLogMsgType::SeriousWarningMsg;
      }
      else if (tmp.StartsWith("Success: "))
      {
        szMsg = &tmp.GetData()[9];
        msgType = plLogMsgType::SuccessMsg;
      }
      else if (tmp.StartsWith("+++++ "))
      {
        tmp.Trim("+ ");
        if (tmp.EndsWith("()"))
          tmp.Trim("() ");

        szMsg = tmp.GetData();
        blocks.PushBack(PL_DEFAULT_NEW(LogBlockData, pLogOutput, szMsg));
        continue;
      }
      else if (tmp.StartsWith("----- "))
      {
        if (!blocks.IsEmpty())
          blocks.PopBack();

        continue;
      }
      else
      {
        szMsg = &tmp.GetData()[0];
        msgType = plLogMsgType::InfoMsg;

        // TODO: output all logged data in one big message, if the tool failed
      }

      if (msgType > logLevel || szMsg == nullptr)
        continue;

      plLog::BroadcastLoggingEvent(pLogOutput, msgType, szMsg);
    }

    blocks.Clear();
  }

  if (proc.exitStatus() == QProcess::ExitStatus::CrashExit)
  {
    return plStatus(plFmt("{0} crashed during execution", szTool));
  }
  else if (proc.exitCode() != 0)
  {
    return plStatus(plFmt("{0} returned error code {1}", szTool, proc.exitCode()));
  }

  return plStatus(PL_SUCCESS);
}

plString plQtEditorApp::BuildFileserveCommandLine() const
{
  const plStringBuilder sToolPath = plQtEditorApp::GetSingleton()->FindToolApplication("Fileserve");
  const plStringBuilder sProjectDir = plToolsProject::GetSingleton()->GetProjectDirectory();
  plStringBuilder params;

  plStringBuilder cmd;
  cmd.Set(sToolPath, " -specialdirs project \"", sProjectDir, "\" -fs_start");

  return cmd;
}

void plQtEditorApp::RunFileserve()
{
  const plStringBuilder sToolPath = plQtEditorApp::GetSingleton()->FindToolApplication("Fileserve");
  const plStringBuilder sProjectDir = plToolsProject::GetSingleton()->GetProjectDirectory();

  QStringList args;
  args << "-specialdirs"
       << "project" << sProjectDir.GetData() << "-fs_start";

  QProcess::startDetached(sToolPath.GetData(), args);
}

void plQtEditorApp::RunInspector()
{
  const plStringBuilder sToolPath = plQtEditorApp::GetSingleton()->FindToolApplication("Inspector");
  QStringList args;

  QProcess::startDetached(sToolPath.GetData(), args);
}
