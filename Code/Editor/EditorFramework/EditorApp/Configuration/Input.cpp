#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

void plQtEditorApp::GetKnownInputSlots(plDynamicArray<plString>& slotList) const
{
  if (slotList.IndexOf("") == plInvalidIndex)
    slotList.PushBack("");

  plStringBuilder sFile;
  plDynamicArray<plStringView> Lines;

  plStringBuilder sSearchDir = plApplicationServices::GetSingleton()->GetApplicationDataFolder();
  sSearchDir.AppendPath("InputSlots/*.txt");

  plFileSystemIterator it;
  for (it.StartSearch(sSearchDir, plFileSystemIteratorFlags::ReportFiles); it.IsValid(); it.Next())
  {
    sFile = it.GetCurrentPath();
    sFile.AppendPath(it.GetStats().m_sName);

    plFileReader reader;
    if (reader.Open(sFile).Succeeded())
    {
      sFile.ReadAll(reader);

      Lines.Clear();
      sFile.Split(false, Lines, "\n", "\r");

      plString sSlot;
      for (plUInt32 s = 0; s < Lines.GetCount(); ++s)
      {
        sSlot = Lines[s];

        if (slotList.IndexOf(sSlot) == plInvalidIndex)
          slotList.PushBack(sSlot);
      }
    }
  }
}
