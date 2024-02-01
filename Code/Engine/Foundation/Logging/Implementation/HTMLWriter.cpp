#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Time/Timestamp.h>

plLogWriter::HTML::~HTML()
{
  EndLog();
}

void plLogWriter::HTML::BeginLog(plStringView sFile, plStringView sAppTitle)
{
  const plUInt32 uiLogCache = 1024 * 10;

  plStringBuilder sNewName;
  if (m_File.Open(sFile.GetData(sNewName), uiLogCache, plFileShareMode::SharedReads) == PL_FAILURE)
  {
    for (plUInt32 i = 1; i < 32; ++i)
    {
      const plStringBuilder sName = plPathUtils::GetFileName(sFile);

      sNewName.SetFormat("{0}_{1}", sName, i);

      plStringBuilder sPath = sFile;
      sPath.ChangeFileName(sNewName);

      if (m_File.Open(sPath.GetData(), uiLogCache) == PL_SUCCESS)
        break;
    }
  }

  if (!m_File.IsOpen())
  {
    plLog::Error("Could not open Log-File \"{0}\".", sFile);
    return;
  }

  plStringBuilder sText;
  sText.SetFormat("<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" content=\"text/html; charset=utf-8\"><TITLE>Log - {0}</TITLE></HEAD><BODY>", sAppTitle);

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
}

void plLogWriter::HTML::EndLog()
{
  if (!m_File.IsOpen())
    return;

  WriteString("", 0);
  WriteString("", 0);
  WriteString(" <<< HTML-Log End >>> ", 0);
  WriteString("", 0);
  WriteString("", 0);

  plStringBuilder sText;
  sText.SetFormat("</BODY></HTML>");

  m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();

  m_File.Close();
}

const plFileWriter& plLogWriter::HTML::GetOpenedLogFile() const
{
  return m_File;
}

void plLogWriter::HTML::SetTimestampMode(plLog::TimestampMode mode)
{
  m_TimestampMode = mode;
}

void plLogWriter::HTML::LogMessageHandler(const plLoggingEventData& eventData)
{
  if (!m_File.IsOpen())
    return;

  plStringBuilder sOriginalText = eventData.m_sText;

  plStringBuilder sTag = eventData.m_sTag;

  // Cannot write <, > or & to HTML, must be escaped
  sOriginalText.ReplaceAll("&", "&amp;");
  sOriginalText.ReplaceAll("<", "&lt;");
  sOriginalText.ReplaceAll(">", "&gt;");
  sOriginalText.ReplaceAll("\n", "<br>\n");

  sTag.ReplaceAll("&", "&amp;");
  sTag.ReplaceAll("<", "&lt;");
  sTag.ReplaceAll(">", "&gt;");

  plStringBuilder sTimestamp;
  plLog::GenerateFormattedTimestamp(m_TimestampMode, sTimestamp);

  bool bFlushWriteCache = false;

  plStringBuilder sText;

  switch (eventData.m_EventType)
  {
    case plLogMsgType::Flush:
      bFlushWriteCache = true;
      break;

    case plLogMsgType::BeginGroup:
      sText.SetFormat("<br><font color=\"#8080FF\"><b> <<< <u>{0}</u> >>> </b> ({1}) </font><br><table width=100%% border=0><tr width=100%%><td "
                   "width=10></td><td width=*>\n",
        sOriginalText, sTag);
      break;

    case plLogMsgType::EndGroup:
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1} sec)>>> </b></font><br><br>\n", sOriginalText, plArgF(eventData.m_fSeconds, 4));
#else
      sText.SetFormat("</td></tr></table><font color=\"#8080FF\"><b> <<< {0} ({1})>>> </b></font><br><br>\n", sOriginalText, "timing info not available");
#endif
      break;

    case plLogMsgType::ErrorMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF0000\"><b><u>Error:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case plLogMsgType::SeriousWarningMsg:
      bFlushWriteCache = true;
      sText.SetFormat("{0}<font color=\"#FF4000\"><b><u>Seriously:</u> {1}</b></font><br>\n", sTimestamp, sOriginalText);
      break;

    case plLogMsgType::WarningMsg:
      sText.SetFormat("{0}<font color=\"#FF8000\"><u>Warning:</u> {1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case plLogMsgType::SuccessMsg:
      sText.SetFormat("{0}<font color=\"#009000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case plLogMsgType::InfoMsg:
      sText.SetFormat("{0}<font color=\"#000000\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case plLogMsgType::DevMsg:
      sText.SetFormat("{0}<font color=\"#3030F0\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    case plLogMsgType::DebugMsg:
      sText.SetFormat("{0}<font color=\"#A000FF\">{1}</font><br>\n", sTimestamp, sOriginalText);
      break;

    default:
      sText.SetFormat("{0}<font color=\"#A0A0A0\">{1}</font><br>\n", sTimestamp, sOriginalText);

      plLog::Warning("Unknown Message Type {1}", eventData.m_EventType);
      break;
  }

  if (!sText.IsEmpty())
  {
    m_File.WriteBytes(sText.GetData(), sizeof(char) * sText.GetElementCount()).IgnoreResult();
  }

  if (bFlushWriteCache)
  {
    m_File.Flush().IgnoreResult();
  }
}

void plLogWriter::HTML::WriteString(plStringView sText, plUInt32 uiColor)
{
  plStringBuilder sTemp;
  sTemp.SetFormat("<font color=\"#{0}\">{1}</font>", plArgU(uiColor, 1, false, 16, true), sText);

  m_File.WriteBytes(sTemp.GetData(), sizeof(char) * sTemp.GetElementCount()).IgnoreResult();
}


