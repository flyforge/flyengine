#include <Foundation/Application/Application.h>
#include <Foundation/IO/Archive/ArchiveBuilder.h>
#include <Foundation/IO/Archive/ArchiveReader.h>
#include <Foundation/IO/Archive/ArchiveUtils.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/CommandLineOptions.h>

/* ArchiveTool command line options:

-out <path>
    Path to a file or folder.

    -out specifies the target to pack or unpack things to.
    For packing mode it has to be a file. The file will be overwritten, if it already exists.
    For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.

    If no -out is specified, it is determined to be where the input file is located.

-unpack <paths>
    One or multiple paths to plArchive files that shall be extracted.

    Example:
      -unpack "path/to/file.plArchive" "another/file.plArchive"

-pack <paths>
    One or multiple paths to folders that shall be packed.

    Example:
      -pack "path/to/folder" "path/to/another/folder"

Description:
    -pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)
    or to unpack multiple archives at the same time.

    If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.
    If all inputs are folders, the mode is 'pack'.
    If all inputs are files, the mode is 'unpack'.

Examples:
    ArchiveTool.exe "C:/Stuff"
      Packs all data in "C:/Stuff" into "C:/Stuff.plArchive"

    ArchiveTool.exe "C:/Stuff" -out "C:/MyStuff.plArchive"
      Packs all data in "C:/Stuff" into "C:/MyStuff.plArchive"

    ArchiveTool.exe "C:/Stuff.plArchive"
      Unpacks all data from the archive into "C:/Stuff"

    ArchiveTool.exe "C:/Stuff.plArchive" -out "C:/MyStuff"
      Unpacks all data from the archive into "C:/MyStuff"
*/

plCommandLineOptionPath opt_Out("_ArchiveTool", "-out", "\
Path to a file or folder.\n\
\n\
-out specifies the target to pack or unpack things to.\n\
For packing mode it has to be a file. The file will be overwritten, if it already exists.\n\
For unpacking, the target should be a folder (may or may not exist) into which the archives get extracted.\n\
\n\
If no -out is specified, it is determined to be where the input file is located.\n\
",
  "");

plCommandLineOptionDoc opt_Unpack("_ArchiveTool", "-unpack", "<paths>", "\
One or multiple paths to plArchive files that shall be extracted.\n\
\n\
Example:\n\
  -unpack \"path/to/file.plArchive\" \"another/file.plArchive\"\n\
",
  "");

plCommandLineOptionDoc opt_Pack("_ArchiveTool", "-pack", "<paths>", "\
One or multiple paths to folders that shall be packed.\n\
\n\
Example:\n\
  -pack \"path/to/folder\" \"path/to/another/folder\"\n\
",
  "");

plCommandLineOptionDoc opt_Desc("_ArchiveTool", "Description:", "", "\
-pack and -unpack can take multiple inputs to either aggregate multiple folders into one archive (pack)\n\
or to unpack multiple archives at the same time.\n\
\n\
If neither -pack nor -unpack is specified, the mode is detected automatically from the list of inputs.\n\
If all inputs are folders, the mode is 'pack'.\n\
If all inputs are files, the mode is 'unpack'.\n\
",
  "");

plCommandLineOptionDoc opt_Examples("_ArchiveTool", "Examples:", "", "\
ArchiveTool.exe \"C:/Stuff\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/Stuff.plArchive\"\n\
\n\
ArchiveTool.exe \"C:/Stuff\" -out \"C:/MyStuff.plArchive\"\n\
  Packs all data in \"C:/Stuff\" into \"C:/MyStuff.plArchive\"\n\
\n\
ArchiveTool.exe \"C:/Stuff.plArchive\"\n\
  Unpacks all data from the archive into \"C:/Stuff\"\n\
\n\
ArchiveTool.exe \"C:/Stuff.plArchive\" -out \"C:/MyStuff\"\n\
  Unpacks all data from the archive into \"C:/MyStuff\"\n\
",
  "");

class plArchiveBuilderImpl : public plArchiveBuilder
{
protected:
  virtual void WriteFileResultCallback(plUInt32 uiCurEntry, plUInt32 uiMaxEntries, plStringView sSourceFile, plUInt64 uiSourceSize, plUInt64 uiStoredSize, plTime duration) const override
  {
    const plUInt64 uiPercentage = (uiSourceSize == 0) ? 100 : (uiStoredSize * 100 / uiSourceSize);
    plLog::Info(" [{}%%] {} ({}%%) - {}", plArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile, uiPercentage, duration);
  }
};

class plArchiveReaderImpl : public plArchiveReader
{
public:
protected:
  virtual bool ExtractNextFileCallback(plUInt32 uiCurEntry, plUInt32 uiMaxEntries, plStringView sSourceFile) const override
  {
    plLog::Info(" [{}%%] {}", plArgU(100 * uiCurEntry / uiMaxEntries, 2), sSourceFile);
    return true;
  }


  virtual bool ExtractFileProgressCallback(plUInt64 bytesWritten, plUInt64 bytesTotal) const override
  {
    // plLog::Dev("   {}%%", plArgU(100 * bytesWritten / bytesTotal));
    return true;
  }
};

class plArchiveTool : public plApplication
{
public:
  using SUPER = plApplication;

  enum class ArchiveMode
  {
    Auto,
    Pack,
    Unpack,
  };

  ArchiveMode m_Mode = ArchiveMode::Auto;

  plDynamicArray<plString> m_sInputs;
  plString m_sOutput;

  plArchiveTool()
    : plApplication("ArchiveTool")
  {
  }

  plResult ParseArguments()
  {
    if (GetArgumentCount() <= 1)
    {
      plLog::Error("No arguments given");
      return PL_FAILURE;
    }

    plCommandLineUtils& cmd = *plCommandLineUtils::GetGlobalInstance();

    m_sOutput = opt_Out.GetOptionValue(plCommandLineOption::LogMode::Always);

    plStringBuilder path;

    if (cmd.GetStringOptionArguments("-pack") > 0)
    {
      m_Mode = ArchiveMode::Pack;
      const plUInt32 args = cmd.GetStringOptionArguments("-pack");

      if (args == 0)
      {
        plLog::Error("-pack option expects at least one argument");
        return PL_FAILURE;
      }

      for (plUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-pack", a));

        if (!plOSFile::ExistsDirectory(m_sInputs.PeekBack()))
        {
          plLog::Error("-pack input path is not a valid directory: '{}'", m_sInputs.PeekBack());
          return PL_FAILURE;
        }
      }
    }
    else if (cmd.GetStringOptionArguments("-unpack") > 0)
    {
      m_Mode = ArchiveMode::Unpack;
      const plUInt32 args = cmd.GetStringOptionArguments("-unpack");

      if (args == 0)
      {
        plLog::Error("-unpack option expects at least one argument");
        return PL_FAILURE;
      }

      for (plUInt32 a = 0; a < args; ++a)
      {
        m_sInputs.PushBack(cmd.GetAbsolutePathOption("-unpack", a));

        if (!plOSFile::ExistsFile(m_sInputs.PeekBack()))
        {
          plLog::Error("-unpack input file does not exist: '{}'", m_sInputs.PeekBack());
          return PL_FAILURE;
        }
      }
    }
    else
    {
      bool bInputsFolders = true;
      bool bInputsFiles = true;

      for (plUInt32 a = 1; a < GetArgumentCount(); ++a)
      {
        const plStringView sArg = GetArgument(a);

        if (sArg.IsEqual_NoCase("-out"))
          break;

        m_sInputs.PushBack(plOSFile::MakePathAbsoluteWithCWD(sArg));

        if (!plOSFile::ExistsDirectory(m_sInputs.PeekBack()))
          bInputsFolders = false;
        if (!plOSFile::ExistsFile(m_sInputs.PeekBack()))
          bInputsFiles = false;
      }

      if (bInputsFolders && !bInputsFiles)
      {
        m_Mode = ArchiveMode::Pack;
      }
      else if (bInputsFiles && !bInputsFolders)
      {
        m_Mode = ArchiveMode::Unpack;
      }
      else
      {
        plLog::Error("Inputs are ambiguous. Specify only folders for packing or only files for unpacking. Use -out as last argument to "
                     "specify a target.");
        return PL_FAILURE;
      }
    }

    plLog::Info("Mode is: {}", m_Mode == ArchiveMode::Pack ? "pack" : "unpack");
    plLog::Info("Inputs:");

    for (const auto& input : m_sInputs)
    {
      plLog::Info("  '{}'", input);
    }

    plLog::Info("Output: '{}'", m_sOutput);

    return PL_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    // Add the empty data directory to access files via absolute paths
    plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

    plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    // prevent further output during shutdown
    plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);

    SUPER::BeforeCoreSystemsShutdown();
  }

  static plArchiveBuilder::InclusionMode PackFileCallback(plStringView sFile)
  {
    const plStringView ext = plPathUtils::GetFileExtension(sFile);

    if (ext.IsEqual_NoCase("jpg") || ext.IsEqual_NoCase("jpeg") || ext.IsEqual_NoCase("png"))
      return plArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("zip") || ext.IsEqual_NoCase("7z"))
      return plArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("mp3") || ext.IsEqual_NoCase("ogg"))
      return plArchiveBuilder::InclusionMode::Uncompressed;

    if (ext.IsEqual_NoCase("dds"))
      return plArchiveBuilder::InclusionMode::Compress_zstd_fast;

    return plArchiveBuilder::InclusionMode::Compress_zstd_average;
  }

  plResult Pack()
  {
    plArchiveBuilderImpl archive;

    for (const auto& folder : m_sInputs)
    {
      archive.AddFolder(folder, plArchiveCompressionMode::Compressed_zstd, PackFileCallback);
    }

    if (m_sOutput.IsEmpty())
    {
      plStringBuilder sArchive = m_sInputs[0];
      sArchive.Append(".plArchive");

      m_sOutput = sArchive;
    }

    m_sOutput = plOSFile::MakePathAbsoluteWithCWD(m_sOutput);

    plLog::Info("Writing archive to '{}'", m_sOutput);
    if (archive.WriteArchive(m_sOutput).Failed())
    {
      plLog::Error("Failed to write the plArchive");

      return PL_FAILURE;
    }

    return PL_SUCCESS;
  }

  plResult Unpack()
  {
    for (const auto& file : m_sInputs)
    {
      plLog::Info("Extracting archive '{}'", file);

      // if the file has a custom archive file extension, just register it as 'allowed'
      // we assume that the user only gives us files that are plArchives
      if (!plArchiveUtils::IsAcceptedArchiveFileExtensions(plPathUtils::GetFileExtension(file)))
      {
        plArchiveUtils::GetAcceptedArchiveFileExtensions().PushBack(plPathUtils::GetFileExtension(file));
      }

      plArchiveReaderImpl reader;
      PL_SUCCEED_OR_RETURN(reader.OpenArchive(file));

      plStringBuilder sOutput = m_sOutput;

      if (sOutput.IsEmpty())
      {
        sOutput = file;
        sOutput.RemoveFileExtension();
      }

      if (reader.ExtractAllFiles(sOutput).Failed())
      {
        plLog::Error("File extraction failed.");
        return PL_FAILURE;
      }
    }

    return PL_SUCCESS;
  }

  virtual Execution Run() override
  {
    {
      plStringBuilder cmdHelp;
      if (plCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, plCommandLineOption::LogAvailableModes::IfHelpRequested, "_ArchiveTool"))
      {
        plLog::Print(cmdHelp);
        return plApplication::Execution::Quit;
      }
    }

    plStopwatch sw;

    if (ParseArguments().Failed())
    {
      SetReturnCode(1);
      return plApplication::Execution::Quit;
    }

    if (m_Mode == ArchiveMode::Pack)
    {
      if (Pack().Failed())
      {
        plLog::Error("Packaging files failed");
        SetReturnCode(2);
      }

      plLog::Success("Finished packing archive in {}", sw.GetRunningTotal());
      return plApplication::Execution::Quit;
    }

    if (m_Mode == ArchiveMode::Unpack)
    {
      if (Unpack().Failed())
      {
        plLog::Error("Extracting files failed");
        SetReturnCode(3);
      }

      plLog::Success("Finished extracting archive in {}", sw.GetRunningTotal());
      return plApplication::Execution::Quit;
    }

    plLog::Error("Unknown mode");
    return plApplication::Execution::Quit;
  }
};

PL_CONSOLEAPP_ENTRY_POINT(plArchiveTool);
