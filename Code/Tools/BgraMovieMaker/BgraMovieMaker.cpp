#include <Foundation/Algorithm/Sorting.h>
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
#include <Texture/Image/Image.h>


class plBgraMovieMakerApp : public plApplication
{
private:
  plString m_sSearchDir;
  plString m_outputFilePath;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;

public:
  typedef plApplication SUPER;

  plBgraMovieMakerApp()
    : plApplication("BgraMovieMaker")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const plLoggingEventData& eventData)
  {
    plBgraMovieMakerApp* app = (plBgraMovieMakerApp*)plApplication::GetApplicationInstance();

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

  virtual void AfterCoreSystemsStartup() override
  {
    plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
    plGlobalLog::AddLogWriter(LogInspector);

    if (GetArgumentCount() < 2)
      plLog::Error("This tool requires at leas one command-line argument: An absolute path to the top-level folder of a library.");

    // Add the empty data directory to access files via absolute paths
    plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    plStringBuilder sSearchDir;

    auto numArgs = GetArgumentCount();
    for (plUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = plStringView(GetArgument(argi));
      if (arg == "-o")
      {
        if (argi + 1 >= numArgs)
        {
          plLog::Error("Missing argument for -o");
          return;
        }
        argi++;
        m_outputFilePath = GetArgument(argi);
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
          plLog::Error("Currently only one directory is supported for searching.");
        }
      }
    }

    if (!plPathUtils::IsAbsolutePath(sSearchDir.GetData()))
    {
      plStringBuilder absPath = plOSFile::GetCurrentWorkingDirectory();
      absPath.AppendPath(sSearchDir);
      absPath.MakeCleanPath();
      sSearchDir = absPath;
    }

    if (!plPathUtils::IsAbsolutePath(m_outputFilePath))
    {
      plStringBuilder absPath = plOSFile::GetCurrentWorkingDirectory();
      absPath.AppendPath(m_outputFilePath);
      absPath.MakeCleanPath();
      m_outputFilePath = absPath;
    }

    m_sSearchDir = sSearchDir;
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

    plGlobalLog::RemoveLogWriter(LogInspector);
    plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual plApplication::Execution Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return plApplication::Execution::Quit;

    plStringBuilder search = m_sSearchDir;
    search.AppendPath("*.png");

    plFileSystemIterator fileIt;
    fileIt.StartSearch(search, plFileSystemIteratorFlags::ReportFiles);

    plDynamicArray<plString> files;
    for (; fileIt.IsValid(); fileIt.Next())
    {
      fileIt.GetStats().GetFullPath(search);
      search.MakeCleanPath();
      files.PushBack(search);
    }

    files.Sort();

    plOSFile outputFile;
    if (outputFile.Open(m_outputFilePath, plFileOpenMode::Write).Failed())
    {
      plLog::Error("Failed to open output file {} for writing", m_outputFilePath);
      return plApplication::Execution::Quit;
    }

    for (plString& file : files)
    {
      plImage img;
      if (img.LoadFrom(file).Failed())
      {
        plLog::Error("failed to load file {}", file);
        break;
      }

      if (img.Convert(plImageFormat::B8G8R8A8_UNORM).Failed())
      {
        plLog::Error("Failed to convert file {}", file);
        break;
      }

      auto byteBlob = img.GetByteBlobPtr();
      if (outputFile.Write(byteBlob.GetPtr(), byteBlob.GetCount()).Failed())
      {
        plLog::Error("Failed to write data to {}", m_outputFilePath);
        break;
      }
    }

    plLog::Info("Successfully wrote rgba movie to {}", m_outputFilePath);

    return plApplication::Execution::Quit;
  }
};

PLASMA_CONSOLEAPP_ENTRY_POINT(plBgraMovieMakerApp);