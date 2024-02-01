#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/CodeGen/CppSettings.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

plResult plCppSettings::Save(plStringView sFile)
{
  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("Target", "Default");

  plOpenDdlUtils::StoreString(ddl, m_sPluginName, "PluginName");

  ddl.EndObject();

  return PL_SUCCESS;
}

plResult plCppSettings::Load(plStringView sFile)
{
  plFileReader file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  plOpenDdlReader ddl;
  PL_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  if (auto pTarget = ddl.GetRootElement()->FindChildOfType("Target", "Default"))
  {
    if (auto pValue = pTarget->FindChildOfType(plOpenDdlPrimitiveType::String, "PluginName"))
    {
      m_sPluginName = pValue->GetPrimitivesString()[0];
    }
  }

  return PL_SUCCESS;
}
