#include <Foundation/Application/Application.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/System/StackTracer.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#include <DbgHelp.h>
#include <Foundation/IO/JSONWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/EnvironmentVariableUtils.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

struct Module
{
  plString m_sFilePath;
  plUInt64 m_uiBaseAddress;
  plUInt32 m_uiSize;
};

struct Stackframe
{
  plUInt32 m_uiModuleIndex = 0xFFFFFFFF;
  plUInt32 m_uiLineNumber = 0;
  plString m_sFilename;
  plString m_sSymbol;
};

plCommandLineOptionString opt_ModuleList("_app", "-ModuleList", "List of modules as a string in this format:\n\n\
File1Path?File1BaseAddressHEX?File1Size|File2Path?File2BaseAddressHEX?File2Size|...\n\n\
For example:\n\
  $[A]/app.exe?7FF7E5540000?106496|$[S]/System32/KERNELBASE.dll?7FFE2B780000?2920448\n\n\
  $[A] represents the application directory and will be adjusted as necessary.\n\
  $[S] represents the system root directory and will be adjusted as necessary.",
  "");
plCommandLineOptionString opt_Callstack("_app", "-Callstack", "Callstack in this format:\n\n7FFE2DD6CE74|7FFE2B7AAA86|7FFE034C22D1", "");

plCommandLineOptionEnum opt_OutputFormat("_app", "-Format", "How to output the resolved callstack.", "Text=0|JSON=1", 0);

plCommandLineOptionPath opt_OutputFile("_app", "-File", "The target file where to write the output to.\nIf left empty, the output is printed to the console.", "");

class plStackResolver : public plApplication
{
public:
  using SUPER = plApplication;

  plStackResolver()
    : plApplication("plStackResolver")
  {
  }

  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual plApplication::Execution Run() override;

  plResult LoadModules();
  plResult ParseModules();
  plResult ParseCallstack();

  void ResolveStackFrames();
  void FormatAsText(plStringBuilder& ref_sOutput);
  void FormatAsJSON(plStringBuilder& ref_sOutput);

  HANDLE m_hProcess;
  plDynamicArray<Module> m_Modules;
  plDynamicArray<plUInt64> m_Callstack;
  plDynamicArray<Stackframe> m_Stackframes;
  plStringBuilder m_SystemRootDir;
  plStringBuilder m_ApplicationDir;
};

void plStackResolver::AfterCoreSystemsStartup()
{
  plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

  m_Modules.Reserve(128);
  m_Callstack.Reserve(128);
  m_Stackframes.Reserve(128);
}

void plStackResolver::BeforeCoreSystemsShutdown()
{
  plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
  plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
}

plResult plStackResolver::ParseModules()
{
  const plStringBuilder sModules = opt_ModuleList.GetOptionValue(plCommandLineOption::LogMode::Never);

  plDynamicArray<plStringView> parts;
  sModules.Split(false, parts, "|");

  for (plStringView sModView : parts)
  {
    plStringBuilder sMod = sModView;
    plDynamicArray<plStringView> parts2;
    sMod.Split(false, parts2, "?");

    plUInt64 base;
    if (plConversionUtils::ConvertHexStringToUInt64(parts2[1], base).Failed())
    {
      plLog::Error("Failed to convert HEX string '{}' to UINT64", parts2[1]);
      return PLASMA_FAILURE;
    }

    plStringBuilder sSize = parts2[2];
    plUInt32 size;
    if (plConversionUtils::StringToUInt(sSize, size).Failed())
    {
      plLog::Error("Failed to convert string '{}' to UINT32", sSize);
      return PLASMA_FAILURE;
    }

    plStringBuilder sModuleName = parts2[0];
    sModuleName.ReplaceFirst_NoCase("$[S]", m_SystemRootDir);
    sModuleName.ReplaceFirst_NoCase("$[A]", m_ApplicationDir);
    sModuleName.MakeCleanPath();

    auto& mod = m_Modules.ExpandAndGetRef();
    mod.m_sFilePath = sModuleName;
    mod.m_uiBaseAddress = base;
    mod.m_uiSize = size;
  }

  return PLASMA_SUCCESS;
}

plResult plStackResolver::ParseCallstack()
{
  plStringBuilder sCallstack = opt_Callstack.GetOptionValue(plCommandLineOption::LogMode::Never);

  plDynamicArray<plStringView> parts;
  sCallstack.Split(false, parts, "|");
  for (plStringView sModView : parts)
  {
    plUInt64 base;
    if (plConversionUtils::ConvertHexStringToUInt64(sModView, base).Failed())
    {
      plLog::Error("Failed to convert HEX string '{}' to UINT64", sModView);
      return PLASMA_FAILURE;
    }

    m_Callstack.PushBack(base);
  }

  return PLASMA_SUCCESS;
}

plResult plStackResolver::LoadModules()
{
  if (SymInitialize(m_hProcess, nullptr, FALSE) != TRUE) // TODO specify PDB search path as second parameter?
  {
    plLog::Error("SymInitialize failed");
    return PLASMA_FAILURE;
  }

  for (const auto& curModule : m_Modules)
  {
    if (SymLoadModuleExW(m_hProcess, nullptr, plStringWChar(curModule.m_sFilePath), nullptr, curModule.m_uiBaseAddress, curModule.m_uiSize, nullptr, 0) == 0)
    {
      plLog::Warning("Couldn't load module '{}'", curModule.m_sFilePath);
    }
    else
    {
      plLog::Success("Loaded module '{}'", curModule.m_sFilePath);
    }
  }

  return PLASMA_SUCCESS;
}

void plStackResolver::ResolveStackFrames()
{
  plStringBuilder tmp;

  char buffer[1024];
  for (plUInt32 i = 0; i < m_Callstack.GetCount(); i++)
  {
    DWORD64 symbolAddress = m_Callstack[i];

    _SYMBOL_INFOW& symbolInfo = *(_SYMBOL_INFOW*)buffer;
    plMemoryUtils::ZeroFill(&symbolInfo, 1);
    symbolInfo.SizeOfStruct = sizeof(_SYMBOL_INFOW);
    symbolInfo.MaxNameLen = (PLASMA_ARRAY_SIZE(buffer) - symbolInfo.SizeOfStruct) / sizeof(WCHAR);

    DWORD64 displacement = 0;
    BOOL result = SymFromAddrW(m_hProcess, symbolAddress, &displacement, &symbolInfo);
    if (!result)
    {
      wcscpy_s(symbolInfo.Name, symbolInfo.MaxNameLen, L"<Unknown>");
    }

    IMAGEHLP_LINEW64 lineInfo;
    DWORD displacement2 = static_cast<DWORD>(displacement);
    plMemoryUtils::ZeroFill(&lineInfo, 1);
    lineInfo.SizeOfStruct = sizeof(lineInfo);
    SymGetLineFromAddrW64(m_hProcess, symbolAddress, &displacement2, &lineInfo);

    auto& frame = m_Stackframes.ExpandAndGetRef();

    for (plUInt32 modIndex = 0; modIndex < m_Modules.GetCount(); modIndex++)
    {
      if (m_Modules[modIndex].m_uiBaseAddress == (plUInt64)symbolInfo.ModBase)
      {
        frame.m_uiModuleIndex = modIndex;
        break;
      }
    }

    frame.m_uiLineNumber = (plUInt32)lineInfo.LineNumber;
    frame.m_sSymbol = plStringUtf8(symbolInfo.Name).GetView();

    tmp = plStringUtf8(lineInfo.FileName).GetView();
    tmp.MakeCleanPath();
    frame.m_sFilename = tmp;
  }
}

void plStackResolver::FormatAsText(plStringBuilder& ref_sOutput)
{
  plLog::Info("Formatting callstack as text.");

  for (const auto& frame : m_Stackframes)
  {
    plStringView sModuleName = "<unknown module>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    plStringView sFileName = "<unknown file>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    plStringView sSymbol = "<unknown symbol>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    ref_sOutput.AppendFormat("[][{}] {}({}): '{}'\n", sModuleName, sFileName, frame.m_uiLineNumber, sSymbol);
  }
}

void plStackResolver::FormatAsJSON(plStringBuilder& ref_sOutput)
{
  plLog::Info("Formatting callstack as JSON.");

  plContiguousMemoryStreamStorage storage;
  plMemoryStreamWriter writer(&storage);

  plStandardJSONWriter json;
  json.SetOutputStream(&writer);
  json.SetWhitespaceMode(plJSONWriter::WhitespaceMode::LessIndentation);

  json.BeginObject();
  json.BeginArray("Stackframes");

  for (const auto& frame : m_Stackframes)
  {
    plStringView sModuleName = "<unknown>";

    if (frame.m_uiModuleIndex < m_Modules.GetCount())
    {
      sModuleName = m_Modules[frame.m_uiModuleIndex].m_sFilePath;
    }

    plStringView sFileName = "<unknown>";
    if (!frame.m_sFilename.IsEmpty())
    {
      sFileName = frame.m_sFilename;
    }

    plStringView sSymbol = "<unknown>";
    if (!frame.m_sSymbol.IsEmpty())
    {
      sSymbol = frame.m_sSymbol;
    }

    json.BeginObject();
    json.AddVariableString("Module", sModuleName);
    json.AddVariableString("File", sFileName);
    json.AddVariableUInt32("Line", frame.m_uiLineNumber);
    json.AddVariableString("Symbol", sSymbol);
    json.EndObject();
  }

  json.EndArray();
  json.EndObject();

  plStringView text((const char*)storage.GetData(), storage.GetStorageSize32());

  ref_sOutput.Append(text);
}

plApplication::Execution plStackResolver::Run()
{
  if (plCommandLineOption::LogAvailableOptions(plCommandLineOption::LogAvailableModes::IfHelpRequested, "_app"))
    return Execution::Quit;

  plString sMissingOpt;
  if (plCommandLineOption::RequireOptions("-ModuleList;-Callstack", &sMissingOpt).Failed())
  {
    plLog::Error("Command line option '{}' was not specified.", sMissingOpt);

    plCommandLineOption::LogAvailableOptions(plCommandLineOption::LogAvailableModes::Always, "_app");
    return Execution::Quit;
  }

  m_hProcess = GetCurrentProcess();

  m_ApplicationDir = plOSFile::GetApplicationDirectory();
  m_ApplicationDir.MakeCleanPath();
  m_ApplicationDir.Trim("", "/");

  m_SystemRootDir = plEnvironmentVariableUtils::GetValueString("SystemRoot");
  m_SystemRootDir.MakeCleanPath();
  m_SystemRootDir.Trim("", "/");

  if (ParseModules().Failed())
    return Execution::Quit;

  if (ParseCallstack().Failed())
    return Execution::Quit;

  if (LoadModules().Failed())
    return Execution::Quit;

  ResolveStackFrames();

  plStringBuilder output;

  if (opt_OutputFormat.GetOptionValue(plCommandLineOption::LogMode::Never) == 0)
  {
    FormatAsText(output);
  }
  else if (opt_OutputFormat.GetOptionValue(plCommandLineOption::LogMode::Never) == 1)
  {
    FormatAsJSON(output);
  }

  if (opt_OutputFile.IsOptionSpecified())
  {
    plLog::Info("Writing output to '{}'.", opt_OutputFile.GetOptionValue(plCommandLineOption::LogMode::Never));

    plOSFile file;
    if (file.Open(opt_OutputFile.GetOptionValue(plCommandLineOption::LogMode::Never), plFileOpenMode::Write).Failed())
    {
      plLog::Error("Could not open file for writing: '{}'", opt_OutputFile.GetOptionValue(plCommandLineOption::LogMode::Never));
      return Execution::Quit;
    }

    file.Write(output.GetData(), output.GetElementCount()).IgnoreResult();
  }
  else
  {
    plLog::Info("Writing output to console.");

    PLASMA_LOG_BLOCK("Resolved callstack");

    plDynamicArray<plStringView> lines;
    output.Split(true, lines, "\n");

    for (auto l : lines)
    {
      plLog::Info("{}", l);
    }
  }

  return Execution::Quit;
}

PLASMA_CONSOLEAPP_ENTRY_POINT(plStackResolver);
