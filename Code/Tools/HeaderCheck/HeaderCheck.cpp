#include <Foundation/Application/Application.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>


namespace
{
  PLASMA_ALWAYS_INLINE void SkipWhitespace(plToken& token, plUInt32& i, const plDeque<plToken>& tokens)
  {
    while (token.m_iType == plTokenType::Whitespace)
    {
      token = tokens[++i];
    }
  }

  PLASMA_ALWAYS_INLINE void SkipLine(plToken& token, plUInt32& i, const plDeque<plToken>& tokens)
  {
    while (token.m_iType != plTokenType::Newline && token.m_iType != plTokenType::EndOfFile)
    {
      token = tokens[++i];
    }
  }
} // namespace

class plHeaderCheckApp : public plApplication
{
private:
  plString m_sSearchDir;
  plString m_sProjectName;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;
  plUniquePtr<plStackAllocator<plMemoryTrackingFlags::None>> m_pStackAllocator;
  plDynamicArray<plString> m_IncludeDirectories;

  struct IgnoreInfo
  {
    plHashSet<plString> m_byName;
  };

  IgnoreInfo m_IgnoreTarget;
  IgnoreInfo m_IgnoreSource;

public:
  typedef plApplication SUPER;

  plHeaderCheckApp()
    : plApplication("HeaderCheck")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const plLoggingEventData& eventData)
  {
    plHeaderCheckApp* app = (plHeaderCheckApp*)plApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case plLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case plLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case plLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  plResult ParseArray(const plVariant& value, plHashSet<plString>& dst)
  {
    if (!value.CanConvertTo<plVariantArray>())
    {
      plLog::Error("Expected array");
      return PLASMA_FAILURE;
    }
    auto a = value.Get<plVariantArray>();
    const auto arraySize = a.GetCount();
    for (plUInt32 i = 0; i < arraySize; i++)
    {
      auto& el = a[i];
      if (!el.CanConvertTo<plString>())
      {
        plLog::Error("Value {0} at index {1} can not be converted to a string. Expected array of strings.", el, i);
        return PLASMA_FAILURE;
      }
      plStringBuilder file = el.Get<plString>();
      file.ToLower();
      dst.Insert(file);
    }
    return PLASMA_SUCCESS;
  }

  plResult ParseIgnoreFile(const plStringView ignoreFilePath)
  {
    plJSONReader jsonReader;
    jsonReader.SetLogInterface(plLog::GetThreadLocalLogSystem());

    plFileReader reader;
    plString sIgnoreFilePath = ignoreFilePath;
    if (reader.Open(sIgnoreFilePath).Failed())
    {
      plLog::Error("Failed to open ignore file {0}", ignoreFilePath);
      return PLASMA_FAILURE;
    }

    if (jsonReader.Parse(reader).Failed())
      return PLASMA_FAILURE;

    const plStringView includeTarget = "includeTarget";
    const plStringView includeSource = "includeSource";
    const plStringView byName = "byName";

    auto topLevel = jsonReader.GetTopLevelObject();
    for (auto it = topLevel.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Key() == includeTarget || it.Key() == includeSource)
      {
        IgnoreInfo& info = (it.Key() == includeTarget) ? m_IgnoreTarget : m_IgnoreSource;
        auto inner = it.Value().Get<plVariantDictionary>();
        for (auto it2 = inner.GetIterator(); it2.IsValid(); it2.Next())
        {
          if (it2.Key() == byName)
          {
            if (ParseArray(it2.Value(), info.m_byName).Failed())
            {
              plLog::Error("Failed to parse value of '{0}.{1}'.", it.Key(), it2.Key());
              return PLASMA_FAILURE;
            }
          }
          else
          {
            plLog::Error("Unknown field of '{0}.{1}'", it.Key(), it2.Key());
            return PLASMA_FAILURE;
          }
        }
      }
      else
      {
        plLog::Error("Unknown json member in root object '{0}'", it.Key().GetView());
        return PLASMA_FAILURE;
      }
    }
    return PLASMA_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
    plGlobalLog::AddLogWriter(LogInspector);

    m_pStackAllocator = PLASMA_DEFAULT_NEW(plStackAllocator<plMemoryTrackingFlags::None>, "Temp Allocator", plFoundation::GetAlignedAllocator());

    if (GetArgumentCount() < 2)
      plLog::Error("This tool requires at leas one command-line argument: An absolute path to the top-level folder of a library.");

    // Add the empty data directory to access files via absolute paths
    plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    plStringBuilder sSearchDir;

    auto numArgs = GetArgumentCount();
    auto shortInclude = plStringView("-i");
    auto longInclude = plStringView("--includeDir");
    auto shortIgnoreFile = plStringView("-f");
    auto longIgnoreFile = plStringView("--ignoreFile");
    for (plUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = plStringView(GetArgument(argi));
      if (arg == shortInclude || arg == longInclude)
      {
        if (numArgs <= argi + 1)
        {
          plLog::Error("Missing path for {0}", arg);
          return;
        }
        plStringBuilder includeDir = GetArgument(argi + 1);
        if (includeDir == shortInclude || includeDir == longInclude || includeDir == shortIgnoreFile || includeDir == longIgnoreFile)
        {
          plLog::Error("Missing path for {0} found {1} instead", arg, includeDir.GetView());
          return;
        }
        argi++;
        includeDir.MakeCleanPath();
        m_IncludeDirectories.PushBack(includeDir);
      }
      else if (arg == shortIgnoreFile || arg == longIgnoreFile)
      {
        if (numArgs <= argi + 1)
        {
          plLog::Error("Missing path for {0}", arg);
          return;
        }
        plStringBuilder ignoreFile = GetArgument(argi + 1);
        if (ignoreFile == shortInclude || ignoreFile == longInclude || ignoreFile == shortIgnoreFile || ignoreFile == longIgnoreFile)
        {
          plLog::Error("Missing path for {0} found {1} instead", arg, ignoreFile.GetView());
          return;
        }
        argi++;
        ignoreFile.MakeCleanPath();
        if (ParseIgnoreFile(ignoreFile.GetView()).Failed())
          return;
      }
      else
      {
        if (sSearchDir.IsEmpty())
        {
          sSearchDir = arg;
          sSearchDir.MakeCleanPath();
        }
        else
        {
          plLog::Error("Currently only one directory is supported for searching. Did you forget -i|--includeDir?");
        }
      }
    }

    if (!plPathUtils::IsAbsolutePath(sSearchDir.GetData()))
      plLog::Error("The given path is not absolute: '{0}'", sSearchDir);

    m_sSearchDir = sSearchDir;

    auto projectStart = m_sSearchDir.GetView().FindLastSubString("/");
    if (projectStart == nullptr)
    {
      plLog::Error("Failed to parse project name from search path {0}", sSearchDir);
      return;
    }
    plStringBuilder projectName = plStringView(projectStart + 1, m_sSearchDir.GetView().GetEndPointer());
    projectName.ToUpper();
    m_sProjectName = projectName;

    // use such a path to write to an absolute file
    // ':abs/C:/some/file.txt"
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      plLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bHadErrors || m_bHadSeriousWarnings)
      SetReturnCode(2);
    else if (m_bHadWarnings)
      SetReturnCode(1);
    else
      SetReturnCode(0);

    m_pStackAllocator = nullptr;

    plGlobalLog::RemoveLogWriter(LogInspector);
    plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
  }

  plResult ReadEntireFile(plStringView sFile, plStringBuilder& ref_sOut)
  {
    ref_sOut.Clear();

    plFileReader File;
    if (File.Open(sFile) == PLASMA_FAILURE)
    {
      plLog::Error("Could not open for reading: '{0}'", sFile);
      return PLASMA_FAILURE;
    }


    plDynamicArray<plUInt8> FileContent;

    plUInt8 Temp[4024];
    plUInt64 uiRead = File.ReadBytes(Temp, PLASMA_ARRAY_SIZE(Temp));

    while (uiRead > 0)
    {
      FileContent.PushBackRange(plArrayPtr<plUInt8>(Temp, (plUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, PLASMA_ARRAY_SIZE(Temp));
    }

    FileContent.PushBack(0);

    if (!plUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      plLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in "
                   "an editor that does not save the file in Utf8 encoding.",
        sFile);
      return PLASMA_FAILURE;
    }

    ref_sOut = (const char*)&FileContent[0];

    return PLASMA_SUCCESS;
  }

  void IterateOverFiles()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    const plUInt32 uiSearchDirLength = m_sSearchDir.GetElementCount() + 1;

    // get a directory iterator for the search directory
    plFileSystemIterator it;
    it.StartSearch(m_sSearchDir.GetData(), plFileSystemIteratorFlags::ReportFilesRecursive);

    if (it.IsValid())
    {
      plStringBuilder currentFile, sExt;

      // while there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // build the absolute path to the current file
        currentFile = it.GetCurrentPath();
        currentFile.AppendPath(it.GetStats().m_sName.GetData());

        // file extensions are always converted to lower-case actually
        sExt = currentFile.GetFileExtension();

        if (sExt.IsEqual_NoCase("h") || sExt.IsEqual_NoCase("inl"))
        {
          plLog::Info("Checking: {}", currentFile);

          PLASMA_LOG_BLOCK("Header", &currentFile.GetData()[uiSearchDirLength]);
          CheckHeaderFile(currentFile);
          m_pStackAllocator->Reset();
        }
      }
    }
    else
      plLog::Error("Could not search the directory '{0}'", m_sSearchDir);
  }

  void CheckInclude(const plStringBuilder& currentFile, const plStringBuilder& includePath, plUInt32 line)
  {
    plStringBuilder absIncludePath(m_pStackAllocator.Borrow());
    bool includeOutside = true;
    if (includePath.IsAbsolutePath())
    {
      for (auto& includeDir : m_IncludeDirectories)
      {
        if (includePath.StartsWith(includeDir))
        {
          includeOutside = false;
          break;
        }
      }
    }
    else
    {
      bool includeFound = false;
      if (includePath.StartsWith("ThirdParty"))
      {
        includeOutside = true;
      }
      else
      {
        for (auto& includeDir : m_IncludeDirectories)
        {
          absIncludePath = includeDir;
          absIncludePath.AppendPath(includePath);
          if (plOSFile::ExistsFile(absIncludePath))
          {
            includeOutside = false;
            break;
          }
        }
      }
    }

    if (includeOutside)
    {
      plStringBuilder includeFileLower = includePath.GetFileNameAndExtension();
      includeFileLower.ToLower();
      plStringBuilder currentFileLower = currentFile.GetFileNameAndExtension();
      currentFileLower.ToLower();

      bool ignore = m_IgnoreTarget.m_byName.Contains(includeFileLower) || m_IgnoreSource.m_byName.Contains(currentFileLower);

      if (!ignore)
      {
        plLog::Error("Including '{0}' in {1}:{2} leaks underlying implementation details. Including system or thirdparty headers in public pl header "
                     "files is not allowed. Please use an interface, factory or pimpl pattern to hide the implementation and avoid the include. See "
                     "the Documentation Chapter 'General->Header Files' for details.",
          includePath.GetView(), currentFile.GetView(), line);
      }
    }
  }

  void CheckHeaderFile(const plStringBuilder& currentFile)
  {
    plStringBuilder fileContents(m_pStackAllocator.Borrow());
    ReadEntireFile(currentFile.GetData(), fileContents).IgnoreResult();

    auto fileDir = currentFile.GetFileDirectory();

    plStringBuilder internalMacroToken(m_pStackAllocator.Borrow());
    internalMacroToken.Append("PLASMA_", m_sProjectName, "_INTERNAL_HEADER");
    auto internalMacroTokenView = internalMacroToken.GetView();

    plTokenizer tokenizer(m_pStackAllocator.Borrow());
    auto dataView = fileContents.GetView();
    tokenizer.Tokenize(plArrayPtr<const plUInt8>(reinterpret_cast<const plUInt8*>(dataView.GetStartPointer()), dataView.GetElementCount()), plLog::GetThreadLocalLogSystem());

    plStringView hash("#");
    plStringView include("include");
    plStringView openAngleBracket("<");
    plStringView closeAngleBracket(">");

    bool isInternalHeader = false;
    auto tokens = tokenizer.GetTokens();
    const auto numTokens = tokens.GetCount();
    for (plUInt32 i = 0; i < numTokens; i++)
    {
      auto curToken = tokens[i];
      while (curToken.m_iType == plTokenType::Whitespace)
      {
        curToken = tokens[++i];
      }
      if (curToken.m_iType == plTokenType::NonIdentifier && curToken.m_DataView == hash)
      {
        do
        {
          curToken = tokens[++i];
        } while (curToken.m_iType == plTokenType::Whitespace);

        if (curToken.m_iType == plTokenType::Identifier && curToken.m_DataView == include)
        {
          auto includeToken = curToken;
          do
          {
            curToken = tokens[++i];
          } while (curToken.m_iType == plTokenType::Whitespace);

          if (curToken.m_iType == plTokenType::String1)
          {
            // #include "bla"
            plStringBuilder absIncludePath(m_pStackAllocator.Borrow());
            plStringBuilder relativePath(m_pStackAllocator.Borrow());
            relativePath = curToken.m_DataView;
            relativePath.Trim("\"");
            relativePath.MakeCleanPath();
            absIncludePath = fileDir;
            absIncludePath.AppendPath(relativePath);

            if (!plOSFile::ExistsFile(absIncludePath))
            {
              plLog::Error("The file '{0}' does not exist. Includes relative to the global include directories should use the #include "
                           "<path/to/file.h> syntax.",
                absIncludePath);
            }
            else if (!isInternalHeader)
            {
              CheckInclude(currentFile, absIncludePath, includeToken.m_uiLine);
            }
          }
          else if (curToken.m_iType == plTokenType::NonIdentifier && curToken.m_DataView == openAngleBracket)
          {
            // #include <bla>
            bool error = false;
            auto startToken = curToken;
            do
            {
              curToken = tokens[++i];
              if (curToken.m_iType == plTokenType::Newline)
              {
                plLog::Error("Non-terminated '<' in #include {0} line {1}", currentFile.GetView(), includeToken.m_uiLine);
                error = true;
                break;
              }
            } while (curToken.m_iType != plTokenType::NonIdentifier || curToken.m_DataView != closeAngleBracket);

            if (error)
            {
              // in case of error skip the malformed line in hopes that we can recover from the error.
              do
              {
                curToken = tokens[++i];
              } while (curToken.m_iType != plTokenType::Newline);
            }
            else if (!isInternalHeader)
            {
              plStringBuilder includePath(m_pStackAllocator.Borrow());
              includePath = plStringView(startToken.m_DataView.GetEndPointer(), curToken.m_DataView.GetStartPointer());
              includePath.MakeCleanPath();
              CheckInclude(currentFile, includePath, startToken.m_uiLine);
            }
          }
          else
          {
            // error
            plLog::Error("Can not parse #include statement in {0} line {1}", currentFile.GetView(), includeToken.m_uiLine);
          }
        }
        else
        {
          while (curToken.m_iType != plTokenType::Newline && curToken.m_iType != plTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
      else
      {
        if (curToken.m_iType == plTokenType::Identifier && curToken.m_DataView == internalMacroTokenView)
        {
          isInternalHeader = true;
        }
        else
        {
          while (curToken.m_iType != plTokenType::Newline && curToken.m_iType != plTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
    }
  }

  virtual plApplication::Execution Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return plApplication::Execution::Quit;

    IterateOverFiles();

    return plApplication::Execution::Quit;
  }
};

PLASMA_CONSOLEAPP_ENTRY_POINT(plHeaderCheckApp);
