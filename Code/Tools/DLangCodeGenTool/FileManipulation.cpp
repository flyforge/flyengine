#include "DLangCodeGenTool.h"
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

void DLangCodeGenTool::FindTargetFiles()
{
  plFileSystemIterator it;
  plStringBuilder path;

  for (it.StartSearch(plFileSystem::GetSdkRootDirectory(), plFileSystemIteratorFlags::ReportFilesRecursive); it.IsValid(); it.Next())
  {
    if (!plPathUtils::HasExtension(it.GetStats().m_sName, ".d"))
      continue;

    it.GetStats().GetFullPath(path);
    m_TargetFiles.Insert(path);
  }
}

void DLangCodeGenTool::ReadFileContent(const char* szFile)
{
  m_sCurFileContent.Clear();

  plFileReader file;
  if (file.Open(szFile).Failed())
  {
    plLog::Error("Couldn't open target file '{}'", szFile);
    return;
  }

  m_sCurFileContent.ReadAll(file);
  m_sCurFileContent.ReplaceAll("\r", "");
}

void DLangCodeGenTool::WriteFileContent(const char* szFile)
{
  plFileWriter file;
  if (file.Open(szFile).Failed())
  {
    plLog::Error("Couldn't open target file '{}'", szFile);
    return;
  }

  file.WriteBytes(m_sCurFileContent.GetData(), m_sCurFileContent.GetElementCount()).AssertSuccess();
}

void DLangCodeGenTool::RemovePreviousAutogen()
{
  const char* szStartOffset = nullptr;

  while (true)
  {
    // find the begin marker
    const char* szBegin = m_sCurFileContent.FindSubString("// CODEGEN-BEGIN:", szStartOffset);

    if (szBegin == nullptr)
      break;

    // find the end marker
    const char* szEnd = m_sCurFileContent.FindSubString("// CODEGEN-END", szBegin);

    // correct the end to the newline before the end marker
    szEnd = plStringUtils::FindLastSubString(szBegin, "\n", nullptr, szEnd);

    // correct the beginning behind the begin marker
    szBegin = m_sCurFileContent.FindSubString("\n", szBegin);

    if (szEnd == nullptr)
    {
      plLog::Error("Found CODEGEN-BEGIN but not matching CODEGEN-END.");
      break;
    }

    m_sCurFileContent.Remove(szBegin, szEnd);

    // we can assume that the string only shrinks and thus memory isn't moved around
    szStartOffset = szBegin;
  }
}

void DLangCodeGenTool::CleanTargetFiles()
{
  PLASMA_LOG_BLOCK("CleanTargetFiles");

  for (const auto& it : m_TargetFiles)
  {
    PLASMA_LOG_BLOCK("CleanTargetFile", it.GetData());

    ReadFileContent(it);
    RemovePreviousAutogen();
    WriteFileContent(it);
  }
}

void DLangCodeGenTool::ProcessAllFiles()
{
  PLASMA_LOG_BLOCK("ProcessAllFiles");

  m_Lua.Clear();
  m_Lua.RegisterCFunction("Struct", &DLangCodeGenTool::Lua_Struct, this);
  m_Lua.RegisterCFunction("Class", &DLangCodeGenTool::Lua_Class, this);
  m_Lua.RegisterCFunction("InterfaceStruct", &DLangCodeGenTool::Lua_InterfaceStruct, this);
  m_Lua.RegisterCFunction("Enum", &DLangCodeGenTool::Lua_Enum, this);
  m_Lua.RegisterCFunction("SynthesizeTemplate", &DLangCodeGenTool::Lua_SynthesizeTemplate, this);
  m_Lua.RegisterCFunction("WhitelistType", &DLangCodeGenTool::Lua_WhitelistType, this);

  plStringBuilder tmp;
  tmp.Format("ValueType = {}; ReferenceType = {};", (int)TargetType::Value, (int)TargetType::Reference);
  m_Lua.ExecuteString(tmp).AssertSuccess();

  for (const auto& it : m_TargetFiles)
  {
    PLASMA_LOG_BLOCK("ProcessFile", it.GetData());

    ReadFileContent(it);
    ProcessFileCommands();
    ProcessFileCodeGen();
    WriteFileContent(it);
  }
}

void DLangCodeGenTool::ProcessFileCodeGen()
{
  uintptr_t uiOffset = 0;
  plStringBuilder sCommand;

  while (true)
  {
    const char* szBegin = m_sCurFileContent.FindSubString("// CODEGEN-BEGIN:", m_sCurFileContent.GetData() + uiOffset);

    if (szBegin == nullptr)
      break;

    const char* szEnd = m_sCurFileContent.FindSubString("\n", szBegin);
    sCommand.SetSubString_FromTo(szBegin + 17, szEnd);
    sCommand.Trim(" \t\n");

    // store relative offset before string gets modified
    uiOffset = (szBegin - m_sCurFileContent.GetData()) + 1;

    // modify string
    m_szCurFileInsertPos = szEnd + 1;

    if (m_Lua.ExecuteString(sCommand).Failed())
    {
      plLog::Error("Bad code generation command: '{}'", sCommand);
    }
  }
}

void DLangCodeGenTool::ProcessFileCommands()
{
  const char* szOffset = nullptr;
  plStringBuilder sCommand;

  while (true)
  {
    const char* szBegin = m_sCurFileContent.FindSubString("// CODEGEN:", szOffset);

    if (szBegin == nullptr)
      break;

    const char* szEnd = m_sCurFileContent.FindSubString("\n", szBegin);
    sCommand.SetSubString_FromTo(szBegin + 11, szEnd);
    sCommand.Trim(" \t\n");
    szOffset = szEnd;

    if (m_Lua.ExecuteString(sCommand).Failed())
    {
      plLog::Error("Bad command: '{}'", sCommand);
    }
  }
}

int DLangCodeGenTool::Lua_Struct(lua_State* context)
{
  plLuaWrapper s(context);
  DLangCodeGenTool* pThis = (DLangCodeGenTool*)s.GetFunctionLightUserData();

  const plString type = s.GetStringParameter(0);

  if (pThis->m_Phase == Phase::GatherInfo)
  {
    pThis->m_pGenerator->WhitelistType(type, TargetType::Value);
  }
  else
  {
    pThis->m_pGenerator->ClearOutput();

    if (pThis->m_pGenerator->GenerateStructure(type, TargetType::Value, false).Failed())
    {
      plLog::Error("Generating struct '{}' failed.", type);
    }
    else
    {
      plStringView output = pThis->m_pGenerator->GetOutput();
      pThis->m_sCurFileContent.Insert(pThis->m_szCurFileInsertPos, output);
    }

    pThis->m_pGenerator->ClearOutput();
  }

  return s.ReturnToScript();
}

int DLangCodeGenTool::Lua_Class(lua_State* context)
{
  plLuaWrapper s(context);
  DLangCodeGenTool* pThis = (DLangCodeGenTool*)s.GetFunctionLightUserData();

  const plString type = s.GetStringParameter(0);

  if (pThis->m_Phase == Phase::GatherInfo)
  {
    pThis->m_pGenerator->WhitelistType(type, TargetType::Reference); // class == 'pointer type'
  }
  else
  {
    pThis->m_pGenerator->ClearOutput();

    if (pThis->m_pGenerator->GenerateStructure(type, TargetType::Reference, false).Failed())
    {
      plLog::Error("Generating class '{}' failed.", type);
    }
    else
    {
      plStringView output = pThis->m_pGenerator->GetOutput();
      pThis->m_sCurFileContent.Insert(pThis->m_szCurFileInsertPos, output);
    }

    pThis->m_pGenerator->ClearOutput();
  }

  return s.ReturnToScript();
}

int DLangCodeGenTool::Lua_InterfaceStruct(lua_State* context)
{
  plLuaWrapper s(context);
  DLangCodeGenTool* pThis = (DLangCodeGenTool*)s.GetFunctionLightUserData();

  const plString type = s.GetStringParameter(0);

  if (pThis->m_Phase == Phase::GatherInfo)
  {
    pThis->m_pGenerator->WhitelistType(type, TargetType::Value); // struct == 'value type'
  }
  else
  {
    pThis->m_pGenerator->ClearOutput();

    if (pThis->m_pGenerator->GenerateStructure(type, TargetType::InterfaceStruct, false).Failed())
    {
      plLog::Error("Generating interface '{}' failed.", type);
    }
    else
    {
      plStringView output = pThis->m_pGenerator->GetOutput();
      pThis->m_sCurFileContent.Insert(pThis->m_szCurFileInsertPos, output);
    }

    pThis->m_pGenerator->ClearOutput();
  }

  return s.ReturnToScript();
}

int DLangCodeGenTool::Lua_Enum(lua_State* context)
{
  plLuaWrapper s(context);
  DLangCodeGenTool* pThis = (DLangCodeGenTool*)s.GetFunctionLightUserData();

  const plString type = s.GetStringParameter(0);

  if (pThis->m_Phase == Phase::GatherInfo)
  {
    pThis->m_pGenerator->WhitelistType(type, TargetType::Value); // struct == 'value type'
  }
  else
  {
    pThis->m_pGenerator->ClearOutput();

    if (pThis->m_pGenerator->GenerateEnum(type).Failed())
    {
      plLog::Error("Generating enum '{}' failed.", type);
    }
    else
    {
      plStringView output = pThis->m_pGenerator->GetOutput();
      pThis->m_sCurFileContent.Insert(pThis->m_szCurFileInsertPos, output);
    }

    pThis->m_pGenerator->ClearOutput();
  }

  return s.ReturnToScript();
}

int DLangCodeGenTool::Lua_SynthesizeTemplate(lua_State* context)
{
  plLuaWrapper s(context);
  DLangCodeGenTool* pThis = (DLangCodeGenTool*)s.GetFunctionLightUserData();

  const plString sTargetType = s.GetStringParameter(0);
  const plString sTemplate1 = s.GetStringParameter(1);
  const plString sTemplate2 = s.GetStringParameter(2);

  if (pThis->m_Phase == Phase::GatherInfo)
  {
    pThis->m_pGenerator->WhitelistType(sTemplate1, TargetType::Value); // assuming that all synthesized types are used as value types
    pThis->m_pGenerator->WhitelistType(sTemplate2, TargetType::Value);

    if (pThis->m_Structure.SynthesizeTemplate(sTargetType, sTemplate1, sTemplate2).Failed())
    {
      plLog::Error("Failed to synthesize template '{}'", sTargetType);
    }
  }

  return s.ReturnToScript();
}

int DLangCodeGenTool::Lua_WhitelistType(lua_State* context)
{
  plLuaWrapper s(context);
  DLangCodeGenTool* pThis = (DLangCodeGenTool*)s.GetFunctionLightUserData();

  const plString sTypeName = s.GetStringParameter(0);
  const TargetType targetType = (TargetType)s.GetIntParameter(1);

  if (pThis->m_Phase == Phase::GatherInfo)
  {
    pThis->m_pGenerator->WhitelistType(sTypeName, targetType);
  }

  return s.ReturnToScript();
}
