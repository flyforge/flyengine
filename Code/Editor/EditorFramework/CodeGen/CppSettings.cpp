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
  PLASMA_SUCCEED_OR_RETURN(file.Open(sFile.GetStartPointer()));

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("Target", "Default");

  plOpenDdlUtils::StoreString(ddl, m_sPluginName, "PluginName");

  switch (m_Compiler)
  {
    case Compiler::None:
      plOpenDdlUtils::StoreString(ddl, "", "Compiler");
      break;
    case Compiler::Vs2019:
      plOpenDdlUtils::StoreString(ddl, "Vs2019", "Compiler");
      break;
    case Compiler::Vs2022:
      plOpenDdlUtils::StoreString(ddl, "Vs2022", "Compiler");
      break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  ddl.EndObject();

  return PLASMA_SUCCESS;
}

plResult plCppSettings::Load(plStringView sFile)
{
  plFileReader file;
  PLASMA_SUCCEED_OR_RETURN(file.Open(sFile.GetStartPointer()));

  plOpenDdlReader ddl;
  PLASMA_SUCCEED_OR_RETURN(ddl.ParseDocument(file));

  if (auto pTarget = ddl.GetRootElement()->FindChildOfType("Target", "Default"))
  {
    if (auto pValue = pTarget->FindChildOfType(plOpenDdlPrimitiveType::String, "PluginName"))
    {
      m_sPluginName = pValue->GetPrimitivesString()[0];
    }

    if (auto pValue = pTarget->FindChildOfType(plOpenDdlPrimitiveType::String, "Compiler"))
    {
      if (pValue->GetPrimitivesString()[0] == "Vs2019")
        m_Compiler = Compiler::Vs2019;
      else if (pValue->GetPrimitivesString()[0] == "Vs2022")
        m_Compiler = Compiler::Vs2022;
      else
        m_Compiler = Compiler::None;
    }
  }

  return PLASMA_SUCCESS;
}
