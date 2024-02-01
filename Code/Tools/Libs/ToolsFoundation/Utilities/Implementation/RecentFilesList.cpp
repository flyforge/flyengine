#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

void plRecentFilesList::Insert(plStringView sFile, plInt32 iContainerWindow)
{
  plStringBuilder sCleanPath = sFile;
  sCleanPath.MakeCleanPath();

  plString s = sCleanPath;

  for (plUInt32 i = 0; i < m_Files.GetCount(); i++)
  {
    if (m_Files[i].m_File == s)
    {
      m_Files.RemoveAtAndCopy(i);
      break;
    }
  }
  m_Files.PushFront(RecentFile(s, iContainerWindow));

  if (m_Files.GetCount() > m_uiMaxElements)
    m_Files.SetCount(m_uiMaxElements);
}

void plRecentFilesList::Save(plStringView sFile)
{
  plDeferredFileWriter File;
  File.SetOutput(sFile);

  for (const RecentFile& file : m_Files)
  {
    plStringBuilder sTemp;
    sTemp.SetFormat("{0}|{1}", file.m_File, file.m_iContainerWindow);
    File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    File.WriteBytes("\n", sizeof(char)).IgnoreResult();
  }

  if (File.Close().Failed())
    plLog::Error("Unable to open file '{0}' for writing!", sFile);
}

void plRecentFilesList::Load(plStringView sFile)
{
  m_Files.Clear();

  plFileReader File;
  if (File.Open(sFile).Failed())
    return;

  plStringBuilder sAllLines;
  sAllLines.ReadAll(File);

  plHybridArray<plStringView, 16> Lines;
  sAllLines.Split(false, Lines, "\n");

  plStringBuilder sTemp, sTemp2;

  for (const plStringView& sv : Lines)
  {
    sTemp = sv;
    plHybridArray<plStringView, 2> Parts;
    sTemp.Split(false, Parts, "|");

    if (!plOSFile::ExistsFile(Parts[0].GetData(sTemp2)))
      continue;

    if (Parts.GetCount() == 1)
    {
      m_Files.PushBack(RecentFile(Parts[0], 0));
    }
    else if (Parts.GetCount() == 2)
    {
      plStringBuilder sContainer = Parts[1];
      plInt32 iContainerWindow = 0;
      plConversionUtils::StringToInt(sContainer, iContainerWindow).IgnoreResult();
      m_Files.PushBack(RecentFile(Parts[0], iContainerWindow));
    }
  }
}
