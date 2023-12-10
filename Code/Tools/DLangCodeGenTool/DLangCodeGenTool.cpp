#include "DLangCodeGenTool.h"
#include "DLangGenerator.h"
#include <Foundation/IO/FileSystem/FileReader.h>

plCommandLineOptionPath opt_Xml("_DLangCodeGenTool", "-xml", "Path to an XML input file.", "");

void DLangCodeGenTool::AfterCoreSystemsStartup()
{
  // Add the empty data directory to access files via absolute paths
  plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
}

void DLangCodeGenTool::BeforeCoreSystemsShutdown()
{
  // prevent further output during shutdown
  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  SUPER::BeforeCoreSystemsShutdown();
}

plApplication::Execution DLangCodeGenTool::Run()
{
  {
    plStringBuilder cmdHelp;
    if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested, "_ArchiveTool"))
    {
      plLog::Print(cmdHelp);
      return plApplication::Execution::Quit;
    }
  }

  if (ParseArguments().Failed())
  {
    SetReturnCode(1);
    return plApplication::Execution::Quit;
  }

  plFileReader file;
  if (file.Open(m_sXmlFile).Failed())
  {
    plLog::Error("Could not read XML file '{0}'", m_sXmlFile);
    SetReturnCode(1);
  }

  if (m_Structure.ParseXML(file).Failed())
  {
    SetReturnCode(4);
    return plApplication::Execution::Quit;
  }

  m_pGenerator = PLASMA_DEFAULT_NEW(DLangGenerator);
  m_pGenerator->SetStructure(m_Structure);

  //m_pGenerator->WhitelistType("plGameObjectId");

  FindTargetFiles();
  CleanTargetFiles();

  m_Phase = Phase::GatherInfo;
  ProcessAllFiles();

  m_Phase = Phase::GenerateCode;
  ProcessAllFiles();

  m_pGenerator->GenerateStructure("plGameObjectId", TargetType::Value).IgnoreResult();

  return plApplication::Execution::Quit;
}

plResult DLangCodeGenTool::ParseArguments()
{
  plStringBuilder xmlPath;
  plFileSystem::ResolveSpecialDirectory(">sdk", xmlPath).IgnoreResult();
  xmlPath.AppendPath("Code/Tools/DLangCodeGenTool/CppExtraction/pl.xml");

  opt_Xml.SetDefaultValue(xmlPath.GetData());
  m_sXmlFile = opt_Xml.GetOptionValue(plCommandLineOption::LogMode::Always);

  if (!plFileSystem::ExistsFile(m_sXmlFile))
  {
    plLog::Error("Input XML file does not exist.");
    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

PLASMA_CONSOLEAPP_ENTRY_POINT(DLangCodeGenTool);
