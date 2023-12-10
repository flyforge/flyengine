#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OSFile.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <shellapi.h>
#endif

static plCommandLineUtils g_pCmdLineInstance;

plCommandLineUtils* plCommandLineUtils::GetGlobalInstance()
{
  return &g_pCmdLineInstance;
}

void plCommandLineUtils::SplitCommandLineString(const char* szCommandString, bool bAddExecutableDir, plDynamicArray<plString>& out_args, plDynamicArray<const char*>& out_argsV)
{
  // Add application dir as first argument as customary on other platforms.
  if (bAddExecutableDir)
  {
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    wchar_t moduleFilename[256];
    GetModuleFileNameW(nullptr, moduleFilename, 256);
    out_args.PushBack(plStringUtf8(moduleFilename).GetData());
#else
    PLASMA_ASSERT_NOT_IMPLEMENTED;
#endif
  }

  // Simple args splitting. Not as powerful as Win32's CommandLineToArgvW.
  const char* currentChar = szCommandString;
  const char* lastEnd = currentChar;
  bool inQuotes = false;
  while (*currentChar != '\0')
  {
    if (*currentChar == '\"')
      inQuotes = !inQuotes;
    else if (*currentChar == ' ' && !inQuotes)
    {
      plStringBuilder path = plStringView(lastEnd, currentChar);
      path.Trim(" \"");
      out_args.PushBack(path);
      lastEnd = currentChar + 1;
    }
    plUnicodeUtils::MoveToNextUtf8(currentChar);
  }

  out_argsV.Reserve(out_argsV.GetCount());
  for (plString& str : out_args)
    out_argsV.PushBack(str.GetData());
}

void plCommandLineUtils::SetCommandLine(plUInt32 uiArgc, const char** pArgv, ArgMode mode /*= UseArgcArgv*/)
{
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
  if (mode == ArgMode::PreferOsArgs)
  {
    SetCommandLine();
    return;
  }
#endif

  m_Commands.Clear();
  m_Commands.Reserve(uiArgc);

  for (plUInt32 i = 0; i < uiArgc; ++i)
    m_Commands.PushBack(pArgv[i]);
}

void plCommandLineUtils::SetCommandLine(plArrayPtr<plString> commands)
{
  m_Commands = commands;
}

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)

void plCommandLineUtils::SetCommandLine()
{
  int argc = 0;

  LPWSTR* argvw = CommandLineToArgvW(::GetCommandLineW(), &argc);

  PLASMA_ASSERT_RELEASE(argvw != nullptr, "CommandLineToArgvW failed");

  plArrayPtr<plStringUtf8> ArgvUtf8 = PLASMA_DEFAULT_NEW_ARRAY(plStringUtf8, argc);
  plArrayPtr<const char*> argv = PLASMA_DEFAULT_NEW_ARRAY(const char*, argc);

  for (plInt32 i = 0; i < argc; ++i)
  {
    ArgvUtf8[i] = argvw[i];
    argv[i] = ArgvUtf8[i].GetData();
  }

  SetCommandLine(argc, argv.GetPtr(), ArgMode::UseArgcArgv);


  PLASMA_DEFAULT_DELETE_ARRAY(ArgvUtf8);
  PLASMA_DEFAULT_DELETE_ARRAY(argv);
  LocalFree(argvw);
}

#elif PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_UWP)
// Not implemented on Windows UWP.
#elif PLASMA_ENABLED(PLASMA_PLATFORM_OSX)
// Not implemented on OSX.
#elif PLASMA_ENABLED(PLASMA_PLATFORM_LINUX)
// Not implemented on Linux.
#elif PLASMA_ENABLED(PLASMA_PLATFORM_ANDROID)
// Not implemented on Android.
#else
#  error "plCommandLineUtils::SetCommandLine(): Abstraction missing."
#endif

const plDynamicArray<plString>& plCommandLineUtils::GetCommandLineArray() const
{
  return m_Commands;
}

plString plCommandLineUtils::GetCommandLineString() const
{
  plStringBuilder commandLine;
  for (const plString& command : m_Commands)
  {
    if (commandLine.IsEmpty())
    {
      commandLine.Append(command.GetView());
    }
    else
    {
      commandLine.Append(" ", command);
    }
  }
  return commandLine;
}

plUInt32 plCommandLineUtils::GetParameterCount() const
{
  return m_Commands.GetCount();
}

const plString& plCommandLineUtils::GetParameter(plUInt32 uiParam) const
{
  return m_Commands[uiParam];
}

plInt32 plCommandLineUtils::GetOptionIndex(plStringView sOption, bool bCaseSensitive) const
{
  PLASMA_ASSERT_DEV(sOption.StartsWith("-"), "All command line option names must start with a hyphen (e.g. -file)");

  for (plUInt32 i = 0; i < m_Commands.GetCount(); ++i)
  {
    if ((bCaseSensitive && m_Commands[i].IsEqual(sOption)) || (!bCaseSensitive && m_Commands[i].IsEqual_NoCase(sOption)))
      return i;
  }

  return -1;
}

bool plCommandLineUtils::HasOption(plStringView sOption, bool bCaseSensitive /*= false*/) const
{
  return GetOptionIndex(sOption, bCaseSensitive) >= 0;
}

plUInt32 plCommandLineUtils::GetStringOptionArguments(plStringView sOption, bool bCaseSensitive) const
{
  const plInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return 0;

  plUInt32 uiParamCount = 0;

  for (plUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> no parameters
      break;

    ++uiParamCount;
  }

  return uiParamCount;
}

plStringView plCommandLineUtils::GetStringOption(plStringView sOption, plUInt32 uiArgument, plStringView sDefault, bool bCaseSensitive) const
{
  const plInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  // not found -> no parameters
  if (iIndex < 0)
    return sDefault;

  plUInt32 uiParamCount = 0;

  for (plUInt32 uiParam = iIndex + 1; uiParam < m_Commands.GetCount(); ++uiParam)
  {
    if (m_Commands[uiParam].StartsWith("-")) // next command is the next option -> not enough parameters
      return sDefault;

    // found the right one, return it
    if (uiParamCount == uiArgument)
      return m_Commands[uiParam].GetData();

    ++uiParamCount;
  }

  return sDefault;
}

const plString plCommandLineUtils::GetAbsolutePathOption(plStringView sOption, plUInt32 uiArgument /*= 0*/, plStringView sDefault /*= {} */, bool bCaseSensitive /*= false*/) const
{
  plStringView sPath = GetStringOption(sOption, uiArgument, sDefault, bCaseSensitive);

  if (sPath.IsEmpty())
    return sPath;

  return plOSFile::MakePathAbsoluteWithCWD(sPath);
}

bool plCommandLineUtils::GetBoolOption(plStringView sOption, bool bDefault, bool bCaseSensitive) const
{
  const plInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return bDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command, treat this as 'on'
    return true;

  if (m_Commands[iIndex + 1].StartsWith("-")) // next command is the next option -> treat this as 'on' as well
    return true;

  // otherwise try to convert the next option to a boolean
  bool bRes = bDefault;
  plConversionUtils::StringToBool(m_Commands[iIndex + 1].GetData(), bRes).IgnoreResult();

  return bRes;
}

plInt32 plCommandLineUtils::GetIntOption(plStringView sOption, plInt32 iDefault, bool bCaseSensitive) const
{
  const plInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return iDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return iDefault;

  // try to convert the next option to a number
  plInt32 iRes = iDefault;
  plConversionUtils::StringToInt(m_Commands[iIndex + 1].GetData(), iRes).IgnoreResult();

  return iRes;
}

plUInt32 plCommandLineUtils::GetUIntOption(plStringView sOption, plUInt32 uiDefault, bool bCaseSensitive) const
{
  const plInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return uiDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return uiDefault;

  // try to convert the next option to a number
  plUInt32 uiRes = uiDefault;
  plConversionUtils::StringToUInt(m_Commands[iIndex + 1].GetData(), uiRes).IgnoreResult();

  return uiRes;
}

double plCommandLineUtils::GetFloatOption(plStringView sOption, double fDefault, bool bCaseSensitive) const
{
  const plInt32 iIndex = GetOptionIndex(sOption, bCaseSensitive);

  if (iIndex < 0)
    return fDefault;

  if (iIndex + 1 == m_Commands.GetCount()) // last command
    return fDefault;

  // try to convert the next option to a number
  double fRes = fDefault;
  plConversionUtils::StringToFloat(m_Commands[iIndex + 1].GetData(), fRes).IgnoreResult();

  return fRes;
}

void plCommandLineUtils::InjectCustomArgument(plStringView sArgument)
{
  m_Commands.PushBack(sArgument);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_CommandLineUtils);
